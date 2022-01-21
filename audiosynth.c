#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <portaudio.h>

#include "linked_list.h"

#define PI 3.14159265

#define SAMPLE_RATE 44100
#define FRAME_LEN 128
#define BPM 120
#define BEAT_LEN ((uint32_t)((double)60 / (double)BPM * SAMPLE_RATE))

struct ChannelState {
    uint32_t waveOffset;
    uint16_t freq;
    double cAmp;
    double tAmp;
};

#define CHANNEL_MOD_TYPE_NOOP 0x00
#define CHANNEL_MOD_TYPE_FREQ 0x01
#define CHANNEL_MOD_TYPE_AMP 0x02

struct ChannelMod {
    uint8_t type;
    uint16_t value;
};

struct ChannelFrame {
    struct LinkedListNode* mods[FRAME_LEN];
};

struct AudioStreamData {
    uint32_t t;
    double amp;
    struct ChannelState channelState;
    struct ChannelFrame frame;
};

static struct AudioStreamData audioStreamData;

static void ChannelFrame_init(struct ChannelFrame* frame)
{
    for (unsigned int i = 0; i < FRAME_LEN; i++)
        frame->mods[i] = NULL;
}

static void ChannelState_default(struct ChannelState* state)
{
    state->waveOffset = 0;
    state->freq = 1;
    state->cAmp = 0.0;
    state->tAmp = 0.0;
}

static double ChannelState_sample(struct ChannelState* state, uint32_t t)
{
    /* ATTACK*/
    if (t % 10 == 0) {
        if (state->tAmp > state->cAmp)
            state->cAmp += 0.01;
        if (state->tAmp < state->cAmp)
            state->cAmp -= 0.01;
    }

    /* WAVE */
    state->waveOffset += state->freq;
    state->waveOffset = state->waveOffset % SAMPLE_RATE;
    double v;
    v = sin(
        ((double)state->waveOffset / (double)SAMPLE_RATE)
        * 2 * PI);

    /* AMPLITUDE */
    v *= state->cAmp;

    return v;
}

static void modChannelState(struct ChannelState* state, struct ChannelMod* mod, uint32_t t)
{
    switch (mod->type) {
    case CHANNEL_MOD_TYPE_NOOP:
        break;
    case CHANNEL_MOD_TYPE_FREQ:
        state->freq = mod->value;
        break;
    case CHANNEL_MOD_TYPE_AMP:
        state->tAmp = (double)mod->value / (double)UINT16_MAX;
        break;
    default:
        break;
    }
}

static void nextBeat(struct AudioStreamData* d, uint32_t t)
{
    uint32_t beat = d->t / BEAT_LEN;
    printf("%d\n", beat);
    struct LinkedListNode* node = d->frame.mods[beat];
    while(node != NULL) {
        modChannelState(&d->channelState, node->data, t);
        node = node->next;
    }
}

static int callback(
    const void* input,
    void* output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    struct AudioStreamData* d = (struct AudioStreamData*)userData;
    float* out = (float*)output;
    for (uint32_t f = 0; f < frameCount; f++) {
        if (d->t % BEAT_LEN == 0)
            nextBeat(d, d->t);
        double v = 0;
        v += ChannelState_sample(&d->channelState, d->t);
        d->t++;
        *out++ = v * d->amp;
    }
    return 0;
}

static void handlePaError(PaErrorCode err, char* msg)
{
    if (err != paNoError) {
        printf("portaudio error. exiting. : %s : %s\n", msg, Pa_GetErrorText(err));
        exit(1);
    }
}

int main()
{
    handlePaError(Pa_Initialize(), "initalizing portaudio");

    PaStream* stream;
    handlePaError(
        Pa_OpenDefaultStream(
            &stream,
            0,
            1,
            paFloat32,
            SAMPLE_RATE,
            256,
            callback,
            &audioStreamData),
        "creating audio stream");

    ChannelState_default(&audioStreamData.channelState);
    audioStreamData.amp = 0.5;
    audioStreamData.t = 0;
    ChannelFrame_init(&audioStreamData.frame);

    struct ChannelMod mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_FREQ, 432 };
    audioStreamData.frame.mods[4] = createLinkedListNode(sizeof(struct ChannelMod));
    *(struct ChannelMod*)audioStreamData.frame.mods[4]->data = mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, (uint16_t)(UINT32_MAX * 0.3) };
    audioStreamData.frame.mods[4]->next = createLinkedListNode(sizeof(struct ChannelMod));
    *(struct ChannelMod*)audioStreamData.frame.mods[4]->next->data = mod;

    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, 0 };
    audioStreamData.frame.mods[8] = createLinkedListNode(sizeof(struct ChannelMod));
    *(struct ChannelMod*)audioStreamData.frame.mods[8]->data = mod;


    handlePaError(
        Pa_StartStream(stream),
        "starting audio stream");

    Pa_Sleep(6000);
    /*
    Pa_Sleep(1000);
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_FREQ, 432 };
    modChannelState(&audioStreamData.channelState, &mod, audioStreamData.t);
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, (uint16_t)(UINT32_MAX * 0.5) };
    modChannelState(&audioStreamData.channelState, &mod, audioStreamData.t);
    Pa_Sleep(1000);
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_FREQ, 432 * 2 / 3 };
    modChannelState(&audioStreamData.channelState, &mod, audioStreamData.t);
    Pa_Sleep(1000);
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, 0 };
    modChannelState(&audioStreamData.channelState, &mod, audioStreamData.t);
    Pa_Sleep(1000);
    */

    handlePaError(
        Pa_StopStream(stream),
        "stopping audio stream");

    handlePaError(
        Pa_CloseStream(stream),
        "closing audio stream");

    handlePaError(
        Pa_Terminate(),
        "terminating audio stream");

    return 0;
}
