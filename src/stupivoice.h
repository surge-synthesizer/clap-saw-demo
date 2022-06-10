/*
 * StupiSaw is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#ifndef STUPISAW_STUPIVOICE_H
#define STUPISAW_STUPIVOICE_H

#include <array>

/*
 * This is a simple voice implementation. You almost definitely don't want to spend
 * much time with it, other than to know it has parameters and when youc all step
 * it updates the L and R value. It is a bad implementatoin of a unison saw and a
 * simple biquad filter.
 */

namespace BaconPaul
{
struct StupiVoice
{
    static constexpr int max_uni = 7;

    int unison;
    float uniSpread;
    int key;
    int noteid;
    enum
    {
        OFF,
        ATTACK,
        HOLD,
        NEWLY_OFF,
        RELEASE
    } state{OFF};

    enum
    {
        DECAY,
        SUSTAIN
    } filterState{DECAY};

    float L, R;
    void step();
    void start(int key);
    void release();
    void recalcRates();

    struct StereoBiQuadLPF
    {
        float b0{1}, b1{0}, b2{0}, a0{1}, a1{0}, a2{0};
        void setCoeff(float key, float res, float srInv);
        void step(float &L, float &R);
        void init();
        float x[2][3], y[2][3];
    } filter;

    std::array<float, max_uni> panL, panR, unitShift, norm;
    std::array<float, max_uni> phase, dPhase;

    float baseFreq{440.0};
    float srInv{1.0 / 44100.0};

    float time{0}, filterTime{0};
    float releaseFrom{1.0};
    float sampleRate{0};

    float cutoff{69.0}, res{0.7};
    float ampAttack{0.01}, ampRelease{0.1};

    float cutoffMod{0.0}, resMod{0.0}, spreadMod{0.0};
};
} // namespace BaconPaul
#endif // STUPISAW_STUPIVOICE_H
