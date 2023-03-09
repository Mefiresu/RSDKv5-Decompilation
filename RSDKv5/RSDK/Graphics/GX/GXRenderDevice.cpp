#include "RSDK/Core/RetroEngine.hpp"
#include <gccore.h>
#include <malloc.h>
#include <wiiuse/wpad.h>

static unsigned char gp_fifo[GX_FIFO_MINSIZE] __attribute__((aligned(32))) = {0};

static void *xfb; // Single buffered
static GXRModeObj *vmode;
static GXTexObj fbTex; // Texture object for the game framebuffer
static uint16 *fbGX; // Framebuffer texture
static int initialWidth; // Width given to the game
static int actualWidth; // Actual game framebuffer width

static int viewWidth; // Wii framebuffer width

#define HASPECT (320)
#define VASPECT (240)

static s16 square[] __attribute__((aligned(32))) = {
    -HASPECT,  VASPECT, 0,
     HASPECT,  VASPECT, 0,
     HASPECT, -VASPECT, 0,
    -HASPECT, -VASPECT, 0
};

//indirect texture matrix for sharp bilinear
static float indtexmtx[2][3] = {
    { +.5, +.0, +.0 },
    { +.0, +.5, +.0 }
};


//indirect texture
static uint16_t indtexdata[][4 * 4] ATTRIBUTE_ALIGN(32) = {
    {
        0xE0E0, 0xA0E0, 0x60E0, 0x20E0,
        0xE0A0, 0xA0A0, 0x60A0, 0x20A0,
        0xE060, 0xA060, 0x6060, 0x2060,
        0xE020, 0xA020, 0x6020, 0x2020,
    }, {
        0xC0C0, 0x40C0, 0xC0C0, 0x40C0,
        0xC040, 0x4040, 0xC040, 0x4040,
        0xC0C0, 0x40C0, 0xC0C0, 0x40C0,
        0xC040, 0x4040, 0xC040, 0x4040,
    }, {
        0x8080, 0x8080, 0x8080, 0x8080,
        0x8080, 0x8080, 0x8080, 0x8080,
        0x8080, 0x8080, 0x8080, 0x8080,
        0x8080, 0x8080, 0x8080, 0x8080,
    }
};

static GXTexObj indtexobj;


static inline void
draw_vert (u8 pos, f32 s, f32 t) {
    f32 scaleFactor = (float)(viewWidth / 2) / (float)actualWidth;
    GX_Position1x8(pos);
    GX_TexCoord2f32(s * scaleFactor, t);
}

static inline void
draw_square() {
    Mtx mv;

    guMtxIdentity(mv);
    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    //texel fix makes uvs 1:1 with texels
    draw_vert(0, 0.0, 0.0);
    draw_vert(1, (float)actualWidth, 0.0);
    draw_vert(2, (float)actualWidth, 240.0);
    draw_vert(3, 0.0, 240.0);
    GX_End();
}


bool RenderDevice::Init() {
    // Initialize the video system
    VIDEO_Init();

    if(!videoSettings.runIn240p) {
        vmode = VIDEO_GetPreferredMode(NULL);

        // Check for aspect ratio
        if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
            vmode->viWidth = 720;
            vmode->fbWidth = 720;
            viewWidth = 848;
        } else { // 4:3
            viewWidth = 640;
        }
        if (vmode == &TVPal576IntDfScale || vmode == &TVPal576ProgScale) {
            vmode->viXOrigin = (VI_MAX_WIDTH_PAL - vmode->viWidth) / 2;
            vmode->viYOrigin = (VI_MAX_HEIGHT_PAL - vmode->viHeight) / 2;
        } else {
            vmode->viXOrigin = (VI_MAX_WIDTH_NTSC - vmode->viWidth) / 2;
            vmode->viYOrigin = (VI_MAX_HEIGHT_NTSC - vmode->viHeight) / 2;
        }
    }
    else {
        vmode = &TVNtsc240Ds;
        viewWidth = 640;
    }

    // Set up the video system with the chosen mode
    VIDEO_Configure(vmode);

    // Allocate the video buffers
    xfb = (u32 *)SYS_AllocateFramebuffer(vmode);
    DCInvalidateRange(xfb, VIDEO_GetFrameBufferSize(vmode));
    xfb = (u32 *)MEM_K0_TO_K1(xfb);
    VIDEO_ClearFrameBuffer(vmode, xfb, COLOR_BLACK);
    VIDEO_SetNextFramebuffer(xfb);

    // Setup console (used for showing printf)
    CON_Init(xfb, 20, 20, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * VI_DISPLAY_PIX_SZ);

    // Show the screen
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (vmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();
    else
        while (VIDEO_GetNextField())
            VIDEO_WaitVSync();

    // Initialize GX
    GX_Init(gp_fifo, sizeof(gp_fifo));

    GXColor background = { 0, 0, 0, 0xff };
    GX_SetCopyClear(background, 0x00ffffff);

    //pixel center fix
    GX_SetViewport(1.0f / 24.0f, 1.0f / 24.0f, vmode->fbWidth, vmode->efbHeight, 0.0f, 1.0f);
    GX_SetScissor(0, 0, vmode->fbWidth, vmode->efbHeight);

    GX_SetDispCopySrc(0, 0, vmode->fbWidth, vmode->efbHeight);
    GX_SetDispCopyYScale((float)vmode->xfbHeight / (float)vmode->efbHeight);
    GX_SetDispCopyDst(vmode->fbWidth, vmode->xfbHeight);
    GX_SetCopyFilter(vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
    GX_SetFieldMode(vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt(vmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    GX_SetDispCopyGamma(GX_GM_1_0);
    GX_SetCullMode(GX_CULL_NONE);

    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);

    Mtx44 p;
    guOrtho(p, VASPECT, -(VASPECT), -(HASPECT), HASPECT, 0, 1);
    GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    //texel center fix
    Mtx m;
    for (int i = GX_TEXCOORD0; i < GX_MAXCOORD; i++) {
        GX_SetTexCoordScaleManually(i, GX_TRUE, 1, 1);
    }

    for (int i = 0; i < 10; i++) {
        float s = 1 << (i + 1);

        guMtxScale(m, s, s, s);
        GX_LoadTexMtxImm(m, GX_TEXMTX0 + i * 3, GX_MTX3x4);
    }

    guMtxTrans(m, 1. / 64., 1. / 64., 0);
    GX_LoadTexMtxImm(m, GX_DTTIDENTITY, GX_MTX3x4);

    for (int i = 0; i < 9; i++) {
        float x = i % 3 - 1;
        float y = i / 3 - 1;

        if (i % 2 == 0) {
            x /= 2;
            y /= 2;
        }

        x += 1. / 64.;
        y += 1. / 64.;

        guMtxTrans(m, x, y, 0);
        GX_LoadTexMtxImm(m, GX_DTTMTX1 + i * 3, GX_MTX3x4);
    }

    //Sharp Bilinear filtering for Widescreen
    if (CONF_GetAspectRatio() == CONF_ASPECT_16_9 & !videoSettings.runIn240p) {
        GX_InitTexObj(&indtexobj, indtexdata, 4, 4, GX_TF_IA8, GX_REPEAT, GX_REPEAT, GX_TRUE);
        GX_InitTexObjLOD(&indtexobj, GX_LIN_MIP_LIN, GX_LINEAR, 0., 2., -1. / 3., GX_FALSE, GX_TRUE, GX_ANISO_4);
        GX_LoadTexObj(&indtexobj, GX_TEXMAP1);

        GX_SetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR);
        GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
        GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
        GX_SetZCompLoc(GX_FALSE);

        GX_SetNumChans(0);
        GX_SetNumTexGens(2);
        GX_SetNumIndStages(1);
        GX_SetNumTevStages(1);

        GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        GX_SetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1);

        GX_SetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD1, GX_TEXMAP1);
        GX_SetIndTexMatrix(GX_ITM_0, indtexmtx, -7);

        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
        GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K3_R);
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_TEXC, GX_CC_ZERO);
        GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GX_SetTevIndirect(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_ITF_8, GX_ITB_STU, GX_ITM_0, GX_ITW_OFF, GX_ITW_OFF, GX_FALSE, GX_FALSE, GX_ITBA_OFF);
    }
    //Nearest filtering for 4:3
    else {
        GX_SetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR);
        GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
        GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
        GX_SetZCompLoc(GX_FALSE);

        GX_SetNumChans(0);
        GX_SetNumTexGens(1);
        GX_SetNumIndStages(0);
        GX_SetNumTevStages(1);

        GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

        GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    }

    GX_SetArray(GX_VA_POS, square, 3 * sizeof(s16));

    GX_InvalidateTexAll();

    // Game stuff
    scanlines = (ScanlineInfo *)malloc(SCREEN_YSIZE * sizeof(ScanlineInfo));
    memset(scanlines, 0, SCREEN_YSIZE * sizeof(ScanlineInfo));

    engine.inFocus = 1;
    videoSettings.windowState = WINDOWSTATE_ACTIVE;
    videoSettings.dimMax = 1.0;
    videoSettings.dimPercent = 1.0;

    // Accommodate for aspect ratio
    initialWidth = viewWidth / 2; // Width given to the game
    actualWidth = (initialWidth + 15) & 0xFFFFFFF0; // Actual framebuffer width (from Drawing.cpp:SetScreenSize())
    RSDK::SetScreenSize(0, initialWidth, SCREEN_YSIZE);

    // Init framebuffer texture
    fbGX = (uint16 *)memalign(32, screens[0].pitch * SCREEN_YSIZE * sizeof(uint16) + 32);
    memset(screens[0].frameBuffer, 0, screens[0].pitch * SCREEN_YSIZE * sizeof(uint16) + 32);
    GX_InitTexObj(&fbTex, fbGX, screens[0].pitch, SCREEN_YSIZE, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    if (CONF_GetAspectRatio() == CONF_ASPECT_16_9 & !videoSettings.runIn240p) {
        GX_InitTexObjFilterMode(&fbTex, GX_LINEAR, GX_LINEAR); // Linear filtering for the sharp bilinear setup
    }
    else {
        GX_InitTexObjFilterMode(&fbTex, GX_NEAR, GX_NEAR); // Nearest filtering for 4:3
    }

    InitInputDevices();
    if (!AudioDevice::Init())
        return false;

    return true;
}

/**
 * Converts the RGB565 framebuffer to a tiled texture format the Wii can support.
 * Equivalent to libogc's MakeTexture565 without the hardcoded texture size.
 */
static inline void
fb_to_tiled_texture(void *dst, const void *src, uint32_t width, uint32_t height) {
    GX_RedirectWriteGatherPipe(dst);
    const uint32_t *src32 = (const uint32_t *)src;
    const uint32_t *tmp_src32;

    for (int y = 0; y < height >> 2; y++) {

        tmp_src32 = src32;
        for (int x = 0; x < width >> 2; x++) {
            uint32_t width_2 = width / 2;
            wgPipe->U32 = src32[0x000];
            wgPipe->U32 = src32[0x001];
            wgPipe->U32 = src32[width_2 + 0x000];
            wgPipe->U32 = src32[width_2 + 0x001];
            wgPipe->U32 = src32[width + 0x000]; // width / 2 * 2
            wgPipe->U32 = src32[width + 0x001]; // width / 2 * 2
            wgPipe->U32 = src32[width_2*3 + 0x000];
            wgPipe->U32 = src32[width_2*3 + 0x001];

            src32 += 2;
        }

        src32 = tmp_src32 + width * 2;
    }
    GX_RestoreWriteGatherPipe();
}

void RenderDevice::CopyFrameBuffer() {
    fb_to_tiled_texture(fbGX, screens[0].frameBuffer, screens[0].pitch, SCREEN_YSIZE);

    // Clear texture objects
    GX_InvVtxCache();
    GX_InvalidateTexAll();
    // DCFlushRange(fbGX, screens[0].pitch * SCREEN_YSIZE * sizeof(uint16));

    GX_LoadTexObj(&fbTex, GX_TEXMAP0);
}

void RenderDevice::FlipScreen()
{
    VIDEO_WaitVSync();
    //tiled rendering code
    //declare pointer to pixel xy in xfb
    uint16_t(*xfb1)[vmode->fbWidth] = (uint16_t (*)[vmode->_gx_rmodeobj::fbWidth])(xfb);
    //loop if XFB height greater than EFB height
    for (int y = 0; y < vmode->xfbHeight; y += vmode->efbHeight) {

        //vertical clamping for the EFB/XFB copy
        uint8_t clamp = GX_CLAMP_NONE;

        if (y == 0)
            clamp |= GX_CLAMP_TOP;
        if (y + vmode->efbHeight == vmode->xfbHeight)
            clamp |= GX_CLAMP_BOTTOM;

        GX_SetCopyClamp(clamp);

        //loop if XFB width greater than EFB width
        for (int x = 0; x < vmode->fbWidth; x += 640) {
            uint16_t efbWidth = MIN(vmode->fbWidth - x, 640);

            //set up scissor region
            GX_SetScissor(x, y - 2, efbWidth, vmode->efbHeight + 4);
            GX_SetScissorBoxOffset(x, y - 2);
            GX_ClearBoundingBox();

            //draw fb texture
            draw_square();

            //copy the buffer
            GX_SetDispCopyFrame2Field(GX_COPY_PROGRESSIVE);
            GX_SetDispCopySrc(0, 2, efbWidth, vmode->efbHeight);
            GX_SetDispCopyDst(vmode->fbWidth, vmode->efbHeight);

            GX_CopyDisp(&xfb1[y][x], GX_TRUE);

            //clear the overlapping lines
            GX_SetDispCopyFrame2Field(0x1);
            GX_SetDispCopySrc(0, 0, efbWidth, 2);
            GX_SetDispCopyDst(0, 0);

            GX_CopyDisp(&xfb1[y][x], GX_TRUE);

            //clear the overlapping lines
            GX_SetDispCopyFrame2Field(0x1);
            GX_SetDispCopySrc(0, 2 + vmode->efbHeight, efbWidth, 2);
            GX_SetDispCopyDst(0, 0);

            GX_CopyDisp(&xfb1[y][x], GX_TRUE);
        }
    }

    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);

    GX_DrawDone();

    VIDEO_Flush();
}

void RenderDevice::Release(bool32 isRefresh) {
    if (scanlines)
        free(scanlines);
    if (fbGX)
        free(fbGX);

    free(xfb);
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
    // Close when pressing home
    // FIXME: Maybe this should go in WiiInputDevice, but stopping the engine here makes more sense
    u16 ButtonsHeldGC = PAD_ButtonsHeld(0);
    if (WPAD_ButtonsHeld(0) & (WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME)
        || (ButtonsHeldGC & PAD_BUTTON_START && ButtonsHeldGC & PAD_TRIGGER_L && ButtonsHeldGC & PAD_TRIGGER_R)) {
        isRunning = false;
    }

    return false;
}

void RenderDevice::InitFPSCap() {

}

bool RenderDevice::CheckFPSCap() {
    // VIDEO_WaitVSync();

    return true;
}

void RenderDevice::UpdateFPSCap() {

}

bool RenderDevice::InitShaders() {
    return true;
}

void RenderDevice::LoadShader(const char *fileName, bool32 linear) {

}
