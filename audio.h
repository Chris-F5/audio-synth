#ifndef AUDIOSYNTH_AUDIO_H
#define AUDIOSYNTH_AUDIO_H

#include <stdint.h>

#define CHANNEL_MOD_TYPE_NOOP 0x00
#define CHANNEL_MOD_TYPE_FREQ 0x01
#define CHANNEL_MOD_TYPE_AMP 0x02

struct ChannelMod {
    uint8_t type;
    uint8_t value;
};

struct ChannelFrame {
    uint32_t len;
    uint32_t modCount;
    struct ChannelMod* mods;
};

void ChannelFrame_init(
    struct ChannelFrame* frame,
    uint32_t len,
    uint32_t modCount);
void ChannelFrame_destroy(struct ChannelFrame* frame);
void audioInit(double amp, uint32_t bpm, struct ChannelFrame* frame);
void audioTerminate();


#endif
