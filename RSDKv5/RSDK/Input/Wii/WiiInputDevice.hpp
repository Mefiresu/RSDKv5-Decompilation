namespace SKU
{

struct InputDeviceWii : InputDevice {
    void UpdateInput();
    void ProcessInput(int32 controllerID);
    void CloseDevice();
};

} // namespace SKU