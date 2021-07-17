//
// Created by Paul Walker on 7/17/21.
//

#ifndef STUPISAW_STUPIVOICE_H
#define STUPISAW_STUPIVOICE_H

#include <array>

namespace BaconPaul
{
struct StupiVoice
{
    static constexpr int max_uni = 7;

    int unison;
    float uniSpread;
    int key;
    enum {
        OFF,
        ATTACK,
        HOLD,
        RELEASE
    } state{OFF};

    float L, R;
    void step();
    void start(int key);
    void release();
    void recalcRates();

    std::array<float,max_uni> panL, panR, unitShift, norm;
    std::array<float, max_uni> phase, dPhase;

    float baseFreq{440.0};
    float srInv{1.0/44100.0};

    float time{0};
    float releaseFrom{1.0};
    float sampleRate{0};

    float ampAttack{0.01}, ampRelease{0.1}, filterDecay{0.2}, filterModDepth{0.0};
};
}
#endif // STUPISAW_STUPIVOICE_H
