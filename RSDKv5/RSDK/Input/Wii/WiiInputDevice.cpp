using namespace RSDK;

void SKU::InputDeviceWii::UpdateInput() {

}

void SKU::InputDeviceWii::ProcessInput(int32 controllerID) {

}

void SKU::InputDeviceWii::CloseDevice() {

}

SKU::InputDeviceWii *RSDK::SKU::InitWiiInputDevice(uint32 id) {
    if (inputDeviceCount >= INPUTDEVICE_COUNT)
        return NULL;

    if (inputDeviceList[inputDeviceCount] && inputDeviceList[inputDeviceCount]->active)
        return NULL;

    if (inputDeviceList[inputDeviceCount])
        delete inputDeviceList[inputDeviceCount];

    inputDeviceList[inputDeviceCount] = new InputDeviceWii();

    InputDeviceWii *device = (InputDeviceWii *)inputDeviceList[inputDeviceCount];

    const char *name = "Wiimote"; // FIXME

    uint8 controllerType = DEVICE_SWITCH_PRO;

    device->active      = true;
    device->disabled    = false;
    device->gamepadType = (DEVICE_API_WII << 16) | (DEVICE_TYPE_CONTROLLER << 8) | (controllerType << 0);
    device->id          = id;

    for (int32 i = 0; i < PLAYER_COUNT; ++i) {
        if (inputSlots[i] == id) {
            inputSlotDevices[i] = device;
            device->isAssigned  = true;
        }
    }

    inputDeviceCount++;
    return device;
}

void RSDK::SKU::InitWiiInputAPI() {

}