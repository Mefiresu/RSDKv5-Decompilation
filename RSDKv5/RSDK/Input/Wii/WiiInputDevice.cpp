#include <wiiuse/wpad.h>

#define JSMAXGC 101.0f //maximum left joystick value for gamecube controller

// Swap two buttons
#define SWAP(button1,button2) \
    do { \
        uint8 tmp = button1; \
        button1 = button2; \
        button2 = tmp; \
    } while (0);

using namespace RSDK;

void RSDK::SKU::InputDeviceWii::UpdateInput() {
    WPAD_ScanPads();

    this->buttonMasksWii = WPAD_ButtonsHeld(this->id - CONT_P1);
    WPADData *data = WPAD_Data(this->id - CONT_P1);
    int type = data->exp.type;
    switch (type) {
        case WPAD_EXP_NONE:
        default:
            this->stateUp     = (this->buttonMasksWii & WPAD_BUTTON_RIGHT) != 0;
            this->stateDown   = (this->buttonMasksWii & WPAD_BUTTON_LEFT) != 0;
            this->stateLeft   = (this->buttonMasksWii & WPAD_BUTTON_UP) != 0;
            this->stateRight  = (this->buttonMasksWii & WPAD_BUTTON_DOWN) != 0;
            this->stateA      = (this->buttonMasksWii & WPAD_BUTTON_1) != 0;
            this->stateB      = (this->buttonMasksWii & WPAD_BUTTON_2) != 0;
            this->stateC      = (this->buttonMasksWii & 0) != 0;
            this->stateX      = (this->buttonMasksWii & WPAD_BUTTON_B) != 0;
            this->stateY      = (this->buttonMasksWii & WPAD_BUTTON_A) != 0;
            this->stateZ      = (this->buttonMasksWii & 0) != 0;
            this->stateStart  = (this->buttonMasksWii & WPAD_BUTTON_PLUS) != 0;
            this->stateSelect = (this->buttonMasksWii & WPAD_BUTTON_MINUS) != 0;
            // Reset analog input to prevent it from triggering a movement
            this->vDelta_L    = 0.0f;
            this->hDelta_L    = 0.0f;
            break;
        case WPAD_EXP_NUNCHUK:
            this->stateUp     = (this->buttonMasksWii & WPAD_BUTTON_UP) != 0;
            this->stateDown   = (this->buttonMasksWii & WPAD_BUTTON_DOWN) != 0;
            this->stateLeft   = (this->buttonMasksWii & WPAD_BUTTON_LEFT) != 0;
            this->stateRight  = (this->buttonMasksWii & WPAD_BUTTON_RIGHT) != 0;
            this->stateA      = (this->buttonMasksWii & WPAD_BUTTON_B) != 0;
            this->stateB      = (this->buttonMasksWii & WPAD_BUTTON_A) != 0;
            this->stateC      = (this->buttonMasksWii & 0) != 0;
            this->stateX      = (this->buttonMasksWii & WPAD_NUNCHUK_BUTTON_C) != 0;
            this->stateY      = (this->buttonMasksWii & WPAD_NUNCHUK_BUTTON_Z) != 0;
            this->stateZ      = (this->buttonMasksWii & 0) != 0;
            this->stateStart  = (this->buttonMasksWii & WPAD_BUTTON_PLUS) != 0;
            this->stateSelect = (this->buttonMasksWii & WPAD_BUTTON_MINUS) != 0;
            this->vDelta_L    = (float)(data->exp.nunchuk.js.pos.y - data->exp.nunchuk.js.center.y) / ((float)data->exp.nunchuk.js.max.y / 2.0f);
            this->hDelta_L    = (float)(data->exp.nunchuk.js.pos.x - data->exp.nunchuk.js.center.x) / ((float)data->exp.nunchuk.js.max.x / 2.0f);
            break;
        case WPAD_EXP_CLASSIC:
            this->stateUp     = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_UP) != 0;
            this->stateDown   = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_DOWN) != 0;
            this->stateLeft   = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_LEFT) != 0;
            this->stateRight  = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_RIGHT) != 0;
            this->stateA      = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_B) != 0;
            this->stateB      = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_A) != 0;
            this->stateC      = (this->buttonMasksWii & 0) != 0;
            this->stateX      = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_Y) != 0;
            this->stateY      = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_X) != 0;
            this->stateZ      = (this->buttonMasksWii & 0) != 0;
            this->stateStart  = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_PLUS) != 0;
            this->stateSelect = (this->buttonMasksWii & WPAD_CLASSIC_BUTTON_MINUS) != 0;
            this->vDelta_L    = (float)(data->exp.classic.ljs.pos.y - data->exp.classic.ljs.center.y) / ((float)data->exp.classic.ljs.max.y / 2.0f);
            this->hDelta_L    = (float)(data->exp.classic.ljs.pos.x - data->exp.classic.ljs.center.x) / ((float)data->exp.classic.ljs.max.x / 2.0f);
            break;
    }
    // checks if a gamecube controller is plugged into the wii
    if (PAD_ScanPads() > 0) {
        this->buttonMasksGC = PAD_ButtonsHeld(0);

        this->stateUp     |= (this->buttonMasksGC & PAD_BUTTON_UP) != 0;
        this->stateDown   |= (this->buttonMasksGC & PAD_BUTTON_DOWN) != 0;
        this->stateLeft   |= (this->buttonMasksGC & PAD_BUTTON_LEFT) != 0;
        this->stateRight  |= (this->buttonMasksGC & PAD_BUTTON_RIGHT) != 0;
        this->stateA      |= (this->buttonMasksGC & PAD_BUTTON_B) != 0;
        this->stateB      |= (this->buttonMasksGC & PAD_BUTTON_A) != 0;
        this->stateC      |= (this->buttonMasksGC & 0) != 0;
        this->stateX      |= (this->buttonMasksGC & PAD_BUTTON_Y) != 0;
        this->stateY      |= (this->buttonMasksGC & PAD_BUTTON_X) != 0;
        this->stateZ      |= (this->buttonMasksGC & 0) != 0;
        this->stateStart  |= (this->buttonMasksGC & PAD_BUTTON_START) != 0;
        this->stateSelect |= (this->buttonMasksGC & PAD_TRIGGER_Z) != 0;
        this->vDelta_L = (float)PAD_StickY(0) / (JSMAXGC / 2.0f);
        this->hDelta_L = (float)PAD_StickX(0) / (JSMAXGC / 2.0f);
    }

#if RETRO_REV0U
    // Swap face buttons when running in legacy mode (since faceButtonFlip is unused on the game side)
    // The dev menu uses it though, so don't swap when it's opened.
    if (engine.version != 5 && RSDK::Legacy::gameMode != RSDK::Legacy::ENGINE_DEVMENU) {
        SWAP(this->stateA, this->stateB);
        SWAP(this->stateX, this->stateY);
    }
#endif

    // Update both
    this->ProcessInput(CONT_ANY);
    this->ProcessInput(this->id);
}

void RSDK::SKU::InputDeviceWii::ProcessInput(int32 controllerID) {
    ControllerState *cont = &controller[controllerID];
    AnalogState *Lstick   = &stickL[controllerID];

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
    Lstick->hDelta = this->hDelta_L;
    Lstick->vDelta = this->vDelta_L;
    Lstick->keyUp.press |= this->vDelta_L > INPUT_DEADZONE;
    Lstick->keyDown.press |= this->vDelta_L < -INPUT_DEADZONE;
    Lstick->keyLeft.press |= this->hDelta_L < -INPUT_DEADZONE;
    Lstick->keyRight.press |= this->hDelta_L > INPUT_DEADZONE;
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
    PAD_Init();
    SKU::InitWiiInputDevice(CONT_P1);
    SKU::InitWiiInputDevice(CONT_P2);
    SKU::InitWiiInputDevice(CONT_P3);
    SKU::InitWiiInputDevice(CONT_P4);
}
