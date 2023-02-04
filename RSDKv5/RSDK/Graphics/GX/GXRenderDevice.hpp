#ifndef GXRENDERDEVICE_HPP
#define GXRENDERDEVICE_HPP
class RenderDevice : public RenderDeviceBase
{
public:
    struct WindowInfo {
    union {
      uint32 width;
      uint32 height;
      uint32 refresh_rate;
    } * displays;
  };

  static WindowInfo displayInfo;

    static bool Init();
    static void CopyFrameBuffer();
    static void FlipScreen();
    static void Release(bool32 isRefresh);

    static void RefreshWindow();
    static void GetWindowSize(int32 *width, int32 *height);

    static void SetupImageTexture(int32 width, int32 height, uint8 *imagePixels);
    static void SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);
    static void SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);
    static void SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);

    static bool ProcessEvents();

    static void InitFPSCap();
    static bool CheckFPSCap();
    static void UpdateFPSCap();

    static bool InitShaders();
    static void LoadShader(const char *fileName, bool32 linear);

    static inline void ShowCursor(bool32 shown) {}
    static inline bool GetCursorPos(Vector2 *pos) { return false; }
    static inline void SetWindowTitle() {}

private:
    static bool SetupRendering();
    static void InitVertexBuffer();
    static bool InitGraphicsAPI();

    static void GetDisplays();
};

struct ShaderEntry : public ShaderEntryBase {
};


#endif /* GXRENDERDEVICE_HPP */
