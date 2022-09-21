#include <gccore.h>
#include <malloc.h>
#include <debug.h>

#define DEFAULT_FIFO_SIZE 256 * 1024
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((aligned(32)));
static GXTexObj texobj;
static Mtx view;

static unsigned int *xfb[2] = { NULL, NULL }; // Double buffered
static int fb = 0; // Current external framebuffer
static GXRModeObj *vmode;
static GXTexObj fbTex; // Texture containing the game framebuffer

#define HASPECT 			320
#define VASPECT 			240
typedef struct tagcamera
{
    guVector pos;
    guVector up;
    guVector view;
}
camera;
static camera cam = {
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.5F, 0.0F},
    {0.0F, 0.0F, -0.5F}
};
static s16 square[] ATTRIBUTE_ALIGN (32) =
{
  /*
   * X,   Y,  Z
   * Values set are for roughly 4:3 aspect
   */
    -HASPECT,  VASPECT, 0,	// 0
     HASPECT,  VASPECT, 0,	// 1
     HASPECT, -VASPECT, 0,	// 2
    -HASPECT, -VASPECT, 0	// 3
};

static inline void
draw_vert (u8 pos, u8 c, f32 s, f32 t)
{
    GX_Position1x8 (pos);
    GX_Color1x8 (c);
    GX_TexCoord2f32 (s, t);
}

static inline void
draw_square (Mtx v)
{
    Mtx m;			// model matrix.
    Mtx mv;			// modelview matrix.

    guMtxIdentity (m);
    guMtxTransApply (m, m, 0, 0, -100);
    guMtxConcat (v, m, mv);

    GX_LoadPosMtxImm (mv, GX_PNMTX0);
    GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
    draw_vert (0, 0, 0.0, 0.0);
    draw_vert (1, 0, 1.0, 0.0);
    draw_vert (2, 0, 1.0, 1.0);
    draw_vert (3, 0, 0.0, 1.0);
    GX_End ();
}


bool RenderDevice::Init() {
    /* Initialise the video system */
    VIDEO_Init();
    vmode = VIDEO_GetPreferredMode(NULL);

    /* Set up the video system with the chosen mode */
    VIDEO_Configure(vmode);

    // Allocate the video buffers
    xfb[0] = (u32 *) SYS_AllocateFramebuffer (vmode);
    xfb[1] = (u32 *) SYS_AllocateFramebuffer (vmode);
    DCInvalidateRange(xfb[0], VIDEO_GetFrameBufferSize(vmode));
    DCInvalidateRange(xfb[1], VIDEO_GetFrameBufferSize(vmode));
    xfb[0] = (u32 *) MEM_K0_TO_K1 (xfb[0]);
    xfb[1] = (u32 *) MEM_K0_TO_K1 (xfb[1]);

    VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
    VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);
    VIDEO_SetNextFramebuffer (xfb[0]);

    // Show the screen.
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (vmode->viTVMode & VI_NON_INTERLACE)
            VIDEO_WaitVSync();
        else
            while (VIDEO_GetNextField())
                VIDEO_WaitVSync();

    CON_Init(xfb[0],20,20,vmode->fbWidth,vmode->xfbHeight,vmode->fbWidth*VI_DISPLAY_PIX_SZ);

    /*** Clear out FIFO area ***/
    memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);

    /*** Initialise GX ***/
    GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);

    GXColor background = { 0, 0, 0, 0xff };
    GX_SetCopyClear (background, 0x00ffffff);

    Mtx44 p;
    int df = 1; // deflicker on/off

    GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale ((f32) vmode->xfbHeight / (f32) vmode->efbHeight);
    GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);

    GX_SetDispCopySrc (0, 0, vmode->fbWidth, vmode->efbHeight);
    GX_SetDispCopyDst (vmode->fbWidth, vmode->xfbHeight);
    GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, (df == 1) ? GX_TRUE : GX_FALSE, vmode->vfilter);

    GX_SetFieldMode (vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt (GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetDispCopyGamma (GX_GM_1_0);
    GX_SetCullMode (GX_CULL_NONE);
    GX_SetBlendMode(GX_BM_BLEND,GX_BL_DSTALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);

    GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate (GX_TRUE);
    GX_SetNumChans(1);

    guOrtho(p, 480/2, -(480/2), -(640/2), 640/2, 100, 1000); // matrix, t, b, l, r, n, f
    GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

    GX_ClearVtxDesc ();
    GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
    GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

    GX_SetNumTexGens (1);
    GX_SetNumChans (0);

    GX_SetTexCoordGen (GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);

    memset (&view, 0, sizeof (Mtx));
    guLookAt(view, &cam.pos, &cam.up, &cam.view);
    GX_LoadPosMtxImm (view, GX_PNMTX0);

    GX_InvVtxCache ();	// update vertex cache

    GX_InitTexObj(&fbTex, screens[0].frameBuffer, 640, 480, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_LoadTexObj (&fbTex, GX_TEXMAP0);

    // Game stuff
    scanlines = (ScanlineInfo *) malloc(SCREEN_YSIZE * sizeof(ScanlineInfo));
    if (!scanlines)
        return false;

    videoSettings.windowed     = false;
    videoSettings.windowWidth  = vmode->fbWidth;
    videoSettings.windowHeight = vmode->xfbHeight;

    engine.inFocus = 1;
    videoSettings.windowState = WINDOWSTATE_ACTIVE;
    videoSettings.dimMax = 1.0;
    videoSettings.dimPercent = 1.0;

    RSDK::SetScreenSize(0, 640, 480);

    memset(screens[0].frameBuffer, 0, 640 * 480 * sizeof(uint16));

    return true;
}

void RenderDevice::CopyFrameBuffer() {}

void RenderDevice::FlipScreen() {
    // clear texture objects
    GX_InvVtxCache();
    GX_InvalidateTexAll();
    DCFlushRange(screens[0].frameBuffer, 640*480*sizeof(uint16));
    GX_LoadTexObj(&fbTex, GX_TEXMAP0);
    draw_square(view);
    GX_SetColorUpdate(GX_TRUE);

    fb ^= 1;  // Toggle framebuffer index
    GX_CopyDisp(xfb[fb], GX_TRUE);
    GX_DrawDone();

    VIDEO_SetNextFramebuffer(xfb[fb]);
    VIDEO_Flush();
}

void RenderDevice::Release(bool32 isRefresh) {
    if (scanlines)
        free(scanlines);
}

void RenderDevice::RefreshWindow() {

}
void RenderDevice::GetWindowSize(int32 *width, int32 *height) {
    if (width)
        *width = vmode->fbWidth;
    if (height)
        *height = vmode->xfbHeight;
}

void RenderDevice::SetupImageTexture(int32 width, int32 height, uint8 *imagePixels) {

}

void RenderDevice::SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                        int32 strideV) {

}

void RenderDevice::SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                        int32 strideV) {

}

void RenderDevice::SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                        int32 strideV) {

}

bool RenderDevice::ProcessEvents() {
    return true;
}

void RenderDevice::InitFPSCap() {

}

bool RenderDevice::CheckFPSCap() {
    VIDEO_WaitVSync();

    return true;
}

void RenderDevice::UpdateFPSCap() {

}

void RenderDevice::LoadShader(const char *fileName, bool32 linear) {

}