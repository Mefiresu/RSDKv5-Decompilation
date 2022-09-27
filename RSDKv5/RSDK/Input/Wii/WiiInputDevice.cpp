#include <wiiuse/wpad.h>

using namespace RSDK;

void RSDK::SKU::InputDeviceWii::UpdateInput() {
    WPAD_ScanPads();

    this->buttonMasks = WPAD_ButtonsHeld(0);
    WPADData *data = WPAD_Data(0);
    int type = data->exp.type;
    switch (type) {
        case WPAD_EXP_NONE:
            this->stateUp     = (this->buttonMasks & WPAD_BUTTON_RIGHT) != 0;
            this->stateDown   = (this->buttonMasks & WPAD_BUTTON_LEFT) != 0;
            this->stateLeft   = (this->buttonMasks & WPAD_BUTTON_UP) != 0;
            this->stateRight  = (this->buttonMasks & WPAD_BUTTON_DOWN) != 0;
            this->stateA      = (this->buttonMasks & WPAD_BUTTON_1) != 0;
            this->stateB      = (this->buttonMasks & WPAD_BUTTON_2) != 0;
            this->stateC      = (this->buttonMasks & 0) != 0;
            this->stateX      = (this->buttonMasks & WPAD_BUTTON_A) != 0;
            this->stateY      = (this->buttonMasks & WPAD_BUTTON_B) != 0;
            this->stateZ      = (this->buttonMasks & 0) != 0;
            this->stateStart  = (this->buttonMasks & WPAD_BUTTON_PLUS) != 0;
            this->stateSelect = (this->buttonMasks & WPAD_BUTTON_MINUS) != 0;
            break;
        case WPAD_EXP_NUNCHUK:
            this->stateUp       = (this->buttonMasks & WPAD_BUTTON_UP) != 0;
            this->stateDown     = (this->buttonMasks & WPAD_BUTTON_DOWN) != 0;
            this->stateLeft     = (this->buttonMasks & WPAD_BUTTON_LEFT) != 0;
            this->stateRight    = (this->buttonMasks & WPAD_BUTTON_RIGHT) != 0;
            this->stateA        = (this->buttonMasks & WPAD_BUTTON_B) != 0;
            this->stateB        = (this->buttonMasks & WPAD_BUTTON_A) != 0;
            this->stateC        = (this->buttonMasks & 0) != 0;
            this->stateX        = (this->buttonMasks & WPAD_NUNCHUK_BUTTON_C) != 0;
            this->stateY        = (this->buttonMasks & WPAD_NUNCHUK_BUTTON_Z) != 0;
            this->stateZ        = (this->buttonMasks & 0) != 0;
            this->stateStart    = (this->buttonMasks & WPAD_BUTTON_PLUS) != 0;
            this->stateSelect   = (this->buttonMasks & WPAD_BUTTON_MINUS) != 0;
            this->stateUp       |= (data->exp.nunchuk.js.pos.y > data->exp.nunchuk.js.center.y + 5) ? 1 : 0;
            this->stateDown     |= (data->exp.nunchuk.js.pos.y < data->exp.nunchuk.js.center.y - 5) ? 1 : 0;
            this->stateLeft     |= (data->exp.nunchuk.js.pos.x < data->exp.nunchuk.js.center.x - 5) ? 1 : 0;
            this->stateRight    |= (data->exp.nunchuk.js.pos.x > data->exp.nunchuk.js.center.x + 5) ? 1 : 0;
            break;
        case WPAD_EXP_CLASSIC:
            this->stateUp       = (this->buttonMasks & WPAD_CLASSIC_BUTTON_UP) != 0;
            this->stateDown     = (this->buttonMasks & WPAD_CLASSIC_BUTTON_DOWN) != 0;
            this->stateLeft     = (this->buttonMasks & WPAD_CLASSIC_BUTTON_LEFT) != 0;
            this->stateRight    = (this->buttonMasks & WPAD_CLASSIC_BUTTON_RIGHT) != 0;
            this->stateA        = (this->buttonMasks & WPAD_CLASSIC_BUTTON_B) != 0;
            this->stateB        = (this->buttonMasks & WPAD_CLASSIC_BUTTON_A) != 0;
            this->stateC        = (this->buttonMasks & 0) != 0;
            this->stateX        = (this->buttonMasks & WPAD_CLASSIC_BUTTON_Y) != 0;
            this->stateY        = (this->buttonMasks & WPAD_CLASSIC_BUTTON_X) != 0;
            this->stateZ        = (this->buttonMasks & 0) != 0;
            this->stateStart    = (this->buttonMasks & WPAD_CLASSIC_BUTTON_PLUS) != 0;
            this->stateSelect   = (this->buttonMasks & WPAD_CLASSIC_BUTTON_MINUS) != 0;
            this->stateUp       |= (data->exp.classic.ljs.pos.y > data->exp.classic.ljs.center.y + 5) ? 1 : 0;
            this->stateDown     |= (data->exp.classic.ljs.pos.y < data->exp.classic.ljs.center.y - 5) ? 1 : 0;
            this->stateLeft     |= (data->exp.classic.ljs.pos.x < data->exp.classic.ljs.center.x - 5) ? 1 : 0;
            this->stateRight    |= (data->exp.classic.ljs.pos.x > data->exp.classic.ljs.center.x + 5) ? 1 : 0;
            break;
        default:
            break;
    }
    // Update both
    this->ProcessInput(CONT_ANY);
    this->ProcessInput(CONT_P1);
}

void RSDK::SKU::InputDeviceWii::ProcessInput(int32 controllerID) {
    ControllerState *cont = &controller[controllerID];

    cont->keyUp.press |= this->stateUp;
    cont->keyDown.press |= this->stateDown;
    cont->keyLeft.press |= this->stateLeft;
    cont->keyRight.press |= this->stateRight;
    cont->keyA.press |= this->stateA;
    cont->keyB.press |= this->stateB;
    cont->keyC.press |= this->stateC;
    cont->keyX.press |= this->stateX;
    cont->keyY.press |= this->stateY;
    cont->keyZ.press |= this->stateZ;
    cont->keyStart.press |= this->stateStart;
    cont->keySelect.press |= this->stateSelect;
}

void RSDK::SKU::InputDeviceWii::CloseDevice() {
    this->active     = false;
    this->isAssigned = false;
    WPAD_Shutdown();
}

RSDK::SKU::InputDeviceWii *RSDK::SKU::InitWiiInputDevice(uint32 id) {
    if (inputDeviceCount >= INPUTDEVICE_COUNT)
        return NULL;

    if (inputDeviceList[inputDeviceCount] && inputDeviceList[inputDeviceCount]->active)
        return NULL;

    if (inputDeviceList[inputDeviceCount])
        delete inputDeviceList[inputDeviceCount];

    inputDeviceList[inputDeviceCount] = new InputDeviceWii();

    InputDeviceWii *device = (InputDeviceWii *)inputDeviceList[inputDeviceCount];

    device->active      = true;
    device->disabled    = false;
    device->gamepadType = (DEVICE_API_WII << 16) | (DEVICE_TYPE_CONTROLLER << 8) | (DEVICE_SWITCH_PRO << 0); // FIXME
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
    WPAD_Init();
    SKU::InitWiiInputDevice(CONT_P1);
}
