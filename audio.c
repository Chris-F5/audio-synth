#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <portaudio.h>

#include "audio.h"

#define PI 3.14159265

#define SAMPLE_RATE 44100

struct ChannelState {
    uint32_t waveOffset;
    uint16_t freq;
    double cAmp;
    double tAmp;
};

struct AudioStreamData {
    uint32_t t;
    double amp;
    uint32_t beatLen;
    struct ChannelState channelState;
    uint32_t beat;
    struct ChannelFrame* frame;
} static paStreamData;

static PaStream* paStream;

static const uint16_t modValueFreq[] = {
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494
};

void ChannelFrame_init(
    struct ChannelFrame* frame,
    uint32_t len,
    uint32_t modCount)
{
    frame->len = len;
    frame->modCount = modCount;
    frame->mods = (struct ChannelMod*)malloc(
        len * modCount * sizeof(struct ChannelMod));
    for (uint32_t i = 0; i < len * modCount; i++) {
        frame->mods[i].type = CHANNEL_MOD_TYPE_NOOP;
        frame->mods[i].value = 0;
    }
}

void ChannelFrame_destroy(struct ChannelFrame* frame)
{
    free(frame->mods);
}

static void defaultChannelState(struct ChannelState* state)
{
    state->waveOffset = 0;
    state->freq = 1;
    state->cAmp = 0.0;
    state->tAmp = 0.0;
}

void setBpm(uint32_t bpm)
{
    paStreamData.beatLen = (double)60 / (double)bpm * SAMPLE_RATE;
}

static void modChannelState(struct ChannelState* state, struct ChannelMod* mod)
{
    switch (mod->type) {
    case CHANNEL_MOD_TYPE_NOOP:
        break;
    case CHANNEL_MOD_TYPE_FREQ:
        state->freq = modValueFreq[mod->value];
        break;
    case CHANNEL_MOD_TYPE_AMP:
        state->tAmp = (double)mod->value / (double)UINT8_MAX;
        break;
    default:
        break;
    }
}

static double sampleChannelState(struct ChannelState* state, uint32_t t)
{
    /* ATTACK*/
    if (state->tAmp > state->cAmp)
        state->cAmp += 0.001;
    if (state->tAmp < state->cAmp)
        state->cAmp -= 0.001;

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

static void nextBeat(struct AudioStreamData* d, uint32_t beat)
{
    for (unsigned int i = 0; i < d->frame->modCount; i++)
        modChannelState(
            &d->channelState,
            &d->frame->mods[beat * d->frame->modCount + i]);
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
        double v = 0;
        v += sampleChannelState(&d->channelState, d->t);
        *out++ = v * d->amp;

        d->t++;
        if (d->t % d->beatLen == 0) {
            d->beat += 1;
            d->beat = d->beat % d->frame->len;
            d->t = 0;
            nextBeat(d, d->beat);
        }
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

void audioInit(double amp, uint32_t bpm, struct ChannelFrame* frame)
{
    paStreamData.t = 0;
    paStreamData.amp = amp;
    setBpm(bpm);
    defaultChannelState(&paStreamData.channelState);
    paStreamData.frame = frame;
    nextBeat(&paStreamData, 0);

    handlePaError(Pa_Initialize(), "initalizing portaudio");

    handlePaError(
        Pa_OpenDefaultStream(
            &paStream,
            0,
            1,
            paFloat32,
            SAMPLE_RATE,
            256,
            callback,
            &paStreamData),
        "creating audio stream");

    handlePaError(
        Pa_StartStream(paStream),
        "starting audio stream");
}

void audioTerminate()
{
    handlePaError(
        Pa_StopStream(paStream),
        "stopping audio stream");

    handlePaError(
        Pa_CloseStream(paStream),
        "closing audio stream");

    handlePaError(
        Pa_Terminate(),
        "terminating audio stream");
}
