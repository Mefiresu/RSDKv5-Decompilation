namespace SKU
{

struct InputDeviceWii : InputDevice {
    void UpdateInput();
    void ProcessInput(int32 controllerID);
    void CloseDevice();

    uint32 buttonMasks;
    uint32 buttonMasksPrev;
    uint8 stateUp;
    uint8 stateDown;
    uint8 stateLeft;
    uint8 stateRight;
    uint8 stateA;
    uint8 stateB;
    uint8 stateC;
    uint8 stateX;
    uint8 stateY;
    uint8 stateZ;
    uint8 stateStart;
    uint8 stateSelect;
    float hDelta_L;
    float vDelta_L;
};

void InitWiiInputAPI();

InputDeviceWii *InitWiiInputDevice(uint32 id);

inline uint32 MakeWiiInputDeviceID(bool isGameCube, int port) {
    uint32 id = port + 1;  // we can't use 0, since that's reserved for INPUT_NONE
    id |= isGameCube << 3;
    return id;
}

inline bool IsGameCubeController(uint32 id) {
    return (id & (1 << 3)) != 0;
}

inline int GetWiiInputDevicePort(uint32 id) {
    return (id & ~(1 << 3)) - 1;
}

} // namespace SKU
