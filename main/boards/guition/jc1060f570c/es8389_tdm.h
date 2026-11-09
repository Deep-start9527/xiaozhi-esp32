#pragma once

#include <cstddef>
#include <cstdint>

namespace guition_es8389 {
inline constexpr int kTdmSlots = 4;
inline constexpr int kAecChannels = 2;

// Hardware layout is M0,R0,M1,R1. Feed one microphone/reference pair to AFE.
inline void ExtractMicReference(const int16_t* tdm, int16_t* mr, size_t frames) {
    for (size_t i = 0; i < frames; ++i) {
        mr[i * kAecChannels] = tdm[i * kTdmSlots];
        mr[i * kAecChannels + 1] = tdm[i * kTdmSlots + 1];
    }
}

// ES8389's normal I2S DAC consumes the first 16 bits of each 32-bit half:
// slots 0 and 2 carry L/R audio; slots 1 and 3 are padding, not RX references.
inline void PackPlayback(const int16_t* mono, int16_t* tdm, size_t frames) {
    for (size_t i = 0; i < frames; ++i) {
        tdm[i * kTdmSlots] = mono[i];
        tdm[i * kTdmSlots + 1] = 0;
        tdm[i * kTdmSlots + 2] = mono[i];
        tdm[i * kTdmSlots + 3] = 0;
    }
}
}  // namespace guition_es8389
