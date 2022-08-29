bool RenderDevice::Init() {
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