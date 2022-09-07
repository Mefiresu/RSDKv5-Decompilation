#include <cstdint>
#include <ogc/texconv.h>
#include <gccore.h>
#include <malloc.h>

#define DEFAULT_FIFO_SIZE 256 * 1024
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((aligned(32)));

static unsigned int *xfb[2] = { NULL, NULL }; // Double buffered
static int fb = 0; // Current external framebuffer
static GXRModeObj *vmode;
static GXTexObj fbTex; // Texture object for the game framebuffer
static uint16 *fbGX; // Framebuffer texture

#define HASPECT 			320
#define VASPECT 			240

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
draw_square ()
{
    Mtx mv;			// modelview matrix.

    guMtxIdentity (mv);
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
    memset (gp_fifo, 0, DEFAULT_FIFO_SIZE);

    /*** Initialise GX ***/
    GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);

    GXColor background = { 0, 0, 0, 0xff };
    GX_SetCopyClear (background, 0x00ffffff);

    Mtx44 p;
    int df = 0; // deflicker on/off

    GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale ((f32) vmode->xfbHeight / (f32) vmode->efbHeight);
    GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);

    GX_SetDispCopySrc (0, 0, vmode->fbWidth, vmode->efbHeight);
    f32 y_scale   = GX_GetYScaleFactor(vmode->efbHeight, vmode->xfbHeight);
    u16 xfbWidth  = VIDEO_PadFramebufferWidth(vmode->fbWidth);
    u16 xfbHeight = GX_SetDispCopyYScale((f32)y_scale);
    GX_SetDispCopyDst((u16)xfbWidth, (u16)xfbHeight);
    GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, (df == 1) ? GX_TRUE : GX_FALSE, vmode->vfilter);

    GX_SetFieldMode (vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetDispCopyGamma (GX_GM_1_0);
    GX_SetCullMode (GX_CULL_NONE);

    GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate (GX_TRUE);

    guOrtho(p, VASPECT, -(VASPECT), -(HASPECT), HASPECT, 0, 100); // matrix, t, b, l, r, n, f
    GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

    GX_ClearVtxDesc ();
    GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
    GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_U8, 0);
    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

    GX_SetNumTexGens (1);
    GX_SetNumChans (0);

    GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    GX_SetTexCoordGen (GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_InvalidateTexAll();

    // Game stuff
    scanlines  = (ScanlineInfo *)malloc(SCREEN_YSIZE * sizeof(ScanlineInfo));
    memset(scanlines, 0, SCREEN_YSIZE * sizeof(ScanlineInfo));

    videoSettings.windowed     = false;
    videoSettings.windowWidth  = 320;
    videoSettings.windowHeight = 240;

    engine.inFocus = 1;
    videoSettings.windowState = WINDOWSTATE_ACTIVE;
    videoSettings.dimMax = 1.0;
    videoSettings.dimPercent = 1.0;

    // Init framebuffer texture
    int width = 320;
    fbGX = (uint16 *)memalign(32, width * SCREEN_YSIZE * sizeof(uint16));
    RSDK::SetScreenSize(0, width, SCREEN_YSIZE);
    memset(screens[0].frameBuffer, 0, width * SCREEN_YSIZE * sizeof(uint16));

    InitInputDevices();

    return true;
}

// Converts the RGB565 framebuffer to a tiled texture format the Wii can support
void fb_to_wii_texture(void *dst, const void *src, int32_t width, int32_t height) {
    uint32_t *dst32 = (uint32_t *)dst;
    const uint32_t *src32 = (const uint32_t *)src;
    const uint32_t *tmp_src32;

    for (int y = 0; y < height >> 2; y++) {

        tmp_src32 = src32;
        for (int x = 0; x < width >> 2; x++) {
            int32_t width_2 = width / 2;
            dst32[0] = src32[0x000];
            dst32[1] = src32[0x001];
            dst32[2] = src32[width_2 + 0x000];
            dst32[3] = src32[width_2 + 0x001];
            dst32[4] = src32[width + 0x000]; // width / 2 * 2
            dst32[5] = src32[width + 0x001]; // width / 2 * 2
            dst32[6] = src32[width_2*3 + 0x000];
            dst32[7] = src32[width_2*3 + 0x001];

            src32 += 2;
            dst32 += 8;
        }

        src32 = tmp_src32 + width*2; // src32 = tmp_src32 + 0x400; // + 1024
    }
}

void RenderDevice::CopyFrameBuffer() {
    fb_to_wii_texture(fbGX, screens[0].frameBuffer, screens[0].pitch, SCREEN_YSIZE);
    //MakeTexture565(screens[0].frameBuffer, fbGX, screens[0].pitch, SCREEN_YSIZE); // FIXME: Understand and implement this better for a width != 512
    GX_InitTexObj(&fbTex, fbGX, screens[0].pitch, SCREEN_YSIZE, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjFilterMode(&fbTex, GX_NEAR, GX_NEAR); // Nearest neighbor filtering

    // clear texture objects
    GX_InvVtxCache();
    GX_InvalidateTexAll();
    DCFlushRange(fbGX, screens[0].pitch * SCREEN_YSIZE * sizeof(uint16));

    GX_LoadTexObj(&fbTex, GX_TEXMAP0);
}

void RenderDevice::FlipScreen() {
    draw_square();

    GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate (GX_TRUE);

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