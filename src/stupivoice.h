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

    enum {
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

    std::array<float,max_uni> panL, panR, unitShift, norm;
    std::array<float, max_uni> phase, dPhase;

    float baseFreq{440.0};
    float srInv{1.0/44100.0};

    float time{0}, filterTime{0};
    float releaseFrom{1.0};
    float sampleRate{0};

    float cutoff{69.0}, res{0.7};
    float ampAttack{0.01}, ampRelease{0.1}, filterDecay{0.2}, filterModDepth{30.0};
};
}
#endif // STUPISAW_STUPIVOICE_H
