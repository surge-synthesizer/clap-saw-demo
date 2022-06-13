/*
 * ClapSawDemo is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#ifndef CLAP_SAW_DEMO_VOICE_H
#define CLAP_SAW_DEMO_VOICE_H

#include <array>
#include "debug-helpers.h"

/*
 * This is a simple voice implementation. You almost definitely don't want to spend
 * much time with it, other than to know it has parameters and when youc all step
 * it updates the L and R value. It is a bad implementatoin of a unison saw and a
 * simple biquad filter.
 */

namespace sst::clap_saw_demo
{
struct SawDemoVoice
{
    static constexpr int max_uni = 7;

    int unison;
    float uniSpread;
    int key;
    int noteid;
    enum AEGMode
    {
        OFF,
        ATTACK,
        HOLD,
        NEWLY_OFF,
        RELEASING
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
    void recalcPitch();
    void recalcFilter();

    struct StereoSimperSVF // thanks to urs @ u-he and andy simper @ cytomic
    {
        float ic1eq[2]{0.f, 0.f}, ic2eq[2]{0.f, 0.f};
        float g{0.f}, k{0.f}, gk{0.f}, a1{0.f}, a2{0.f}, a3{0.f}, ak{0.f};
        enum Mode
        {
            LP,
            HP,
            BP,
            NOTCH,
            PEAK,
            ALL
        } mode{LP};

        float low[2], band[2], high[2], notch[2], peak[2], all[2];
        void setCoeff(float key, float res, float srInv);
        void step(float &L, float &R);
        void init();
    } filter;

    std::array<float, max_uni> panL, panR, unitShift, norm;
    std::array<double, max_uni> phase, dPhase, dPhaseInv;

    double baseFreq{440.0};
    double srInv{1.0 / 44100.0};

    float time{0}, filterTime{0};
    float releaseFrom{1.0};
    float sampleRate{0};

    float cutoff{69.0}, res{0.7};
    float ampAttack{0.01}, ampRelease{0.1};
    bool ampGate{false};
    int filterMode{StereoSimperSVF::Mode::LP};
    float preFilterVCA{1.0};

    float cutoffMod{0.0}, resMod{0.0}, spreadMod{0.0}, preFilterVCAMod{0.0};

    float pitchMod{0.f}, neVolumeAdj{0.f};
};
} // namespace sst::clap_saw_demo
#endif
