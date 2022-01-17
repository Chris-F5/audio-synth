#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <portaudio.h>

#define PI 3.14159265

#define SAMPLE_RATE 44100
#define CONCURRENT_INSTRUMENT_COUNT 16
#define CONCURRENT_NOTE_COUNT 3

struct Instrument {
    uint32_t attack;
    uint32_t decay;
};

struct Note {
    float freq;
    float amp;
    uint32_t sustain;
};

struct AudioData {
    uint32_t t;

    float amp;

    struct Instrument noteInstruments[CONCURRENT_INSTRUMENT_COUNT];

    bool noteMask[CONCURRENT_INSTRUMENT_COUNT][CONCURRENT_NOTE_COUNT];
    struct Note notes[CONCURRENT_INSTRUMENT_COUNT][CONCURRENT_NOTE_COUNT];
    uint32_t noteTimes[CONCURRENT_INSTRUMENT_COUNT][CONCURRENT_NOTE_COUNT];
    bool playNowNotes[CONCURRENT_INSTRUMENT_COUNT][CONCURRENT_NOTE_COUNT];
};

void AudioData_init(struct AudioData* data)
{
    data->t = 0;
    data->amp = 1.0f;
    for (uint32_t i = 0; i < CONCURRENT_INSTRUMENT_COUNT * CONCURRENT_NOTE_COUNT; i++)
        data->noteMask[0][i] = false;
}

void AudioData_playNote(
    struct AudioData* data,
    uint32_t insId,
    uint32_t noteTime,
    bool playNow,
    struct Note* note)
{
    uint32_t noteId;
    for (noteId = 0; noteId < CONCURRENT_NOTE_COUNT; noteId++) {
        if (data->noteMask[insId][noteId] == false) {
            printf("%d\n", noteId);
            break;
        } else if (noteId == CONCURRENT_NOTE_COUNT - 1) {
            puts("cant play this note. exiting");
            exit(1);
        }
    }
    data->noteMask[insId][noteId] = true;
    data->notes[insId][noteId] = *note;
    data->noteTimes[insId][noteId] = noteTime;
    data->playNowNotes[insId][noteId] = playNow;
}

static struct AudioData paStreamData;

static int callback(
    const void* input,
    void* output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    struct AudioData* audioData = (struct AudioData*)userData;
    float* out = (float*)output;
    for (uint32_t f = 0; f < frameCount; f++) {
        double v = 0;
        for (uint32_t i = 0; i < CONCURRENT_INSTRUMENT_COUNT; i++) {
            const uint32_t attack = audioData->noteInstruments[i].attack;
            const uint32_t decay = audioData->noteInstruments[i].decay;
            for (uint32_t n = 0; n < CONCURRENT_NOTE_COUNT; n++)
                if (audioData->noteMask[i][n]) {
                    const uint32_t sustain = audioData->notes[i][n].sustain;
                    const uint32_t noteAmp = audioData->notes[i][n].amp;
                    const uint32_t noteFreq = audioData->notes[i][n].freq;
                    if (audioData->playNowNotes[i][n]) {
                        audioData->noteTimes[i][n] = audioData->t;
                        audioData->playNowNotes[i][n] = false;
                    }
                    const uint32_t nT = audioData->noteTimes[i][n];
                    const int relNT = (int)audioData->t - nT;
                    double attDecAmpMod;
                    if (relNT < attack) {
                        attDecAmpMod = (double)relNT / (double)attack;
                    }else if (relNT > attack + sustain) {
                        attDecAmpMod = (double)((attack + sustain + decay) - relNT) / (double)decay;
                    } else {
                        attDecAmpMod = 1.0;
                    }

                    v += sin(
                             (audioData->t / (float)SAMPLE_RATE)
                             * noteFreq * 2 * PI)
                        * noteAmp * attDecAmpMod;
                    unsigned int endTime
                        = audioData->noteTimes[i][n]
                        + audioData->notes[i][n].sustain
                        + attack + decay;
                    if (audioData->t >= endTime)
                        audioData->noteMask[i][n] = false;
                }
        }
        audioData->t++;
        *out++ = v * audioData->amp;
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
            &paStreamData),
        "creating audio stream");

    AudioData_init(&paStreamData);
    paStreamData.amp = 0.1;
    paStreamData.noteInstruments[0] = (struct Instrument) {3000, 3000};

    handlePaError(
        Pa_StartStream(stream),
        "starting audio stream");

    struct Note note1 = (struct Note) { 523, 1.0f, SAMPLE_RATE * 3 };
    struct Note note2 = (struct Note) { 659, 1.0f, SAMPLE_RATE * 2 };
    struct Note note3 = (struct Note) { 739, 1.0f, SAMPLE_RATE };
    Pa_Sleep(1000);
    AudioData_playNote(&paStreamData, 0, 0, true, &note1);
    Pa_Sleep(1000);
    AudioData_playNote(&paStreamData, 0, 0, true, &note2);
    Pa_Sleep(1000);
    AudioData_playNote(&paStreamData, 0, 0, true, &note3);
    Pa_Sleep(3000);

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
