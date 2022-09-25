#include <ogc/lwp.h>
#include <aesndlib.h>

static lwp_t audio_thread = LWP_THREAD_NULL;
static lwpq_t audio_queue = LWP_TQUEUE_NULL;
static bool thread_running = false;

static AESNDPB *aesnd_pb = NULL;

#define BUFFER_SIZE (DSP_STREAMBUFFER_SIZE)
static s16 audio_buffer[BUFFER_SIZE] __attribute__((aligned(32)));

static void AESNDCallback(AESNDPB *pb, u32 state) {
    if (state == VOICE_STATE_STREAM) {
        // Wake up audio thread
        LWP_ThreadSignal(audio_queue);
    }
}

static void *AudioThread(void *arg) {
    while (thread_running) {
        AudioDevice::ProcessAudioMixing(audio_buffer, BUFFER_SIZE / sizeof(SAMPLE_FORMAT));
        AESND_SetVoiceBuffer(aesnd_pb, audio_buffer, BUFFER_SIZE);

        LWP_ThreadSleep(audio_queue);
    }

    return NULL;
}


bool32 AudioDevice::Init() {
    InitAudioChannels();

    // Setup AESND
    AESND_Init();
    aesnd_pb = AESND_AllocateVoice(AESNDCallback);
    AESND_SetVoiceFrequency(aesnd_pb, AUDIO_FREQUENCY);
    AESND_SetVoiceFormat(aesnd_pb, VOICE_STEREO16);
    AESND_SetVoiceStream(aesnd_pb, true);

    // Create audio thread
    thread_running = true;
    LWP_InitQueue(&audio_queue);
    LWP_CreateThread(&audio_thread, AudioThread, NULL, NULL, 0, 67);

    // Start audio
    AESND_SetVoiceStop(aesnd_pb, false);

    return true;
}

void AudioDevice::Release() {
    AESND_SetVoiceStop(aesnd_pb, true);
    AESND_Pause(true);
    thread_running = false;
    LWP_ThreadSignal(audio_queue);
    LWP_JoinThread(audio_thread, NULL);
    LWP_CloseQueue(audio_queue);
    AESND_FreeVoice(aesnd_pb);

    AudioDeviceBase::Release();
}

void AudioDevice::InitAudioChannels() {
    AudioDeviceBase::InitAudioChannels();
}

void AudioDevice::HandleStreamLoad(ChannelInfo *channel, bool32 async) {
    // FIXME: Support async
    LoadStream(channel);
}