#include "RSDK/Core/RetroEngine.hpp"
#include <gccore.h>
#include <malloc.h>
#include <wiiuse/wpad.h>

static unsigned char gp_fifo[GX_FIFO_MINSIZE] __attribute__((aligned(32))) = {0};

static unsigned int *xfb = { NULL }; // Single buffered
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

static inline void
draw_vert (u8 pos, f32 s, f32 t) {
    f32 scaleFactor = (float)(viewWidth / 2) / (float)actualWidth;
    GX_Position1x8(pos);
    GX_Color1u32(0xffffffff);
    GX_TexCoord2f32(s * scaleFactor, t);
}

static inline void
draw_square() {
    Mtx mv;

    guMtxIdentity(mv);
    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    draw_vert(0, 0.0, 0.0);
    draw_vert(1, 1.0, 0.0);
    draw_vert(2, 1.0, 1.0);
    draw_vert(3, 0.0, 1.0);
    GX_End();
}


bool RenderDevice::Init() {
    // Initialize the video system
    VIDEO_Init();

    if (!videoSettings.runIn240p) {
        vmode = VIDEO_GetPreferredMode(NULL);

        // Check for aspect ratio and width
        if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
            if (videoSettings.width720) {
                vmode->viWidth = 720;
                vmode->fbWidth = 720;
                viewWidth      = 848;
            }
            else {
                vmode->viWidth = 672;
                viewWidth      = 848;
            }
        }
        else { // 4:3
            viewWidth = 640;
        }
        if (vmode == &TVPal576IntDfScale || vmode == &TVPal576ProgScale) {
            vmode->viXOrigin = (VI_MAX_WIDTH_PAL - vmode->viWidth) / 2;
            vmode->viYOrigin = (VI_MAX_HEIGHT_PAL - vmode->viHeight) / 2;
        }
        else {
            vmode->viXOrigin = (VI_MAX_WIDTH_NTSC - vmode->viWidth) / 2;
            vmode->viYOrigin = (VI_MAX_HEIGHT_NTSC - vmode->viHeight) / 2;
        }
    }
    else {
        vmode     = &TVNtsc240Ds;
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

    GX_SetViewport(0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
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
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetArray(GX_VA_POS, square, 3 * sizeof(s16));

    GX_SetNumTexGens(1);
    GX_SetNumChans(1);

    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
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
    fbGX = (uint16 *)memalign(32, screens[0].pitch * SCREEN_YSIZE * sizeof(uint16));
    memset(screens[0].frameBuffer, 0, screens[0].pitch * SCREEN_YSIZE * sizeof(uint16));
    GX_InitTexObj(&fbTex, fbGX, screens[0].pitch, SCREEN_YSIZE, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjFilterMode(&fbTex, GX_NEAR, GX_NEAR); // Nearest neighbor filtering

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
fb_to_tiled_texture(void *dst, const void *src, int32_t width, int32_t height) {
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

        src32 = tmp_src32 + width * 2;
    }
}

void RenderDevice::CopyFrameBuffer() {
    fb_to_tiled_texture(fbGX, screens[0].frameBuffer, screens[0].pitch, SCREEN_YSIZE);

    // Clear texture objects
    GX_InvVtxCache();
    GX_InvalidateTexAll();
    DCFlushRange(fbGX, screens[0].pitch * SCREEN_YSIZE * sizeof(uint16));

    GX_LoadTexObj(&fbTex, GX_TEXMAP0);
}

void RenderDevice::FlipScreen()
{
    if (videoSettings.width720 & CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
        // render tiled
        GX_SetScissor(0, 0 - 2, 640, vmode->efbHeight + 4);
        GX_SetScissorBoxOffset(0, 0 - 2);
        GX_ClearBoundingBox();

        draw_square();

        GX_SetDispCopyFrame2Field(GX_COPY_PROGRESSIVE);
        GX_SetDispCopySrc(0, 2, 640, vmode->efbHeight);
        GX_SetDispCopyDst(vmode->fbWidth, vmode->efbHeight);

        GX_CopyDisp(xfb, GX_TRUE);

        GX_SetDispCopyFrame2Field(0x1);
        GX_SetDispCopySrc(0, 0, 640, 2);
        GX_SetDispCopyDst(0, 0);

        GX_CopyDisp(xfb, GX_TRUE);

        GX_SetDispCopyFrame2Field(0x1);
        GX_SetDispCopySrc(0, 2 + vmode->efbHeight, 640, 2);
        GX_SetDispCopyDst(0, 0);

        GX_CopyDisp(xfb, GX_TRUE);

        GX_SetScissor(640, 0 - 2, 80, vmode->efbHeight + 4);
        GX_SetScissorBoxOffset(640, 0 - 2);
        GX_ClearBoundingBox();

        draw_square();

        GX_SetDispCopyFrame2Field(GX_COPY_PROGRESSIVE);
        GX_SetDispCopySrc(0, 2, 80, vmode->efbHeight);
        GX_SetDispCopyDst(vmode->fbWidth, vmode->efbHeight);

        GX_CopyDisp((void *)(u32)xfb + 640 * 2, GX_TRUE);

        GX_SetDispCopyFrame2Field(0x1);
        GX_SetDispCopySrc(0, 0, 80, 2);
        GX_SetDispCopyDst(0, 0);

        GX_CopyDisp((void *)(u32)xfb + 640 * 2, GX_TRUE);

        GX_SetDispCopyFrame2Field(0x1);
        GX_SetDispCopySrc(0, 2 + vmode->efbHeight, 80, 2);
        GX_SetDispCopyDst(0, 0);

        GX_CopyDisp((void *)(u32)xfb + 640 * 2, GX_TRUE);
    }
    else {
        // render normal
        draw_square();
        GX_CopyDisp(xfb, GX_TRUE);
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
    VIDEO_WaitVSync();

    return true;
}

void RenderDevice::UpdateFPSCap() {

}

bool RenderDevice::InitShaders() {
    return true;
}

void RenderDevice::LoadShader(const char *fileName, bool32 linear) {

}
