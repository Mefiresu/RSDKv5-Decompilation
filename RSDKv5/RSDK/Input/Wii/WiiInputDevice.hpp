namespace SKU
{

struct InputDeviceWii : InputDevice {
    void UpdateInput();
    void ProcessInput(int32 controllerID);
    void CloseDevice();
};

void InitWiiInputAPI();

InputDeviceWii *InitWiiInputDevice(uint32 id);

} // namespace SKU