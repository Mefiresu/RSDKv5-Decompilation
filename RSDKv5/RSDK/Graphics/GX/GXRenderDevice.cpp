#include <gccore.h>

static void *xfb;
static GXRModeObj *rmode;

bool RenderDevice::Init() {
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();
    return true;
}

void RenderDevice::CopyFrameBuffer() {

}

void RenderDevice::FlipScreen() {

}

void RenderDevice::Release(bool32 isRefresh) {

}

void RenderDevice::RefreshWindow() {

}
void RenderDevice::GetWindowSize(int32 *width, int32 *height) {

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
    return true;
}

void RenderDevice::UpdateFPSCap() {

}

void RenderDevice::LoadShader(const char *fileName, bool32 linear) {

}