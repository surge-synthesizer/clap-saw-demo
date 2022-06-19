/*
 * ClapSawDemo
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

#ifndef CLAP_SAW_DEMO_VOICE_H
#define CLAP_SAW_DEMO_VOICE_H

#include <array>
#include "debug-helpers.h"

namespace sst::clap_saw_demo
{
/*
 * SawDemoVoice is a single voice with the following features
 *
 * - A saw wave generated using a second derivative of a cubic curve
 * - Internal unison from 1-7 with detuning from 0 - 100 cents
 * - A simple AR envelope; and an independent VCA level
 * - A multi-mode SVF filter
 *
 * It is intended to have 'base' values nad 'modulated' values each
 * of which can be adjusted as a voice is playing.
 *
 * The voice has the peculiar feature that you can fully bypass
 * the amplitude envelope generator and then it will simply gate
 * for note hold duration + release time. This allows externally
 * polyphonic and note expression modulation of the pre-filter VCA
 * without the internal AEG getting in the way.
 *
 * The API is pretty direct, exposing members which you can just write
 * to from the Audio thread.
 */
struct SawDemoVoice
{
    static constexpr int max_uni = 7;

    int portid;  // clap note port index
    int channel; // midi channel
    int key;     // The midi key which triggered me
    int note_id; // and the note_id delivered by the host (used for note expressions)

    // unison count is snapped at voice on
    int unison{3};

    // Note the pattern that we have an item and its modulator as the API
    float uniSpread{10.0}, uniSpreadMod{0.0};

    // The oscillator detuning
    float oscDetune{0}, oscDetuneMod{0};

    // Filter characteristics. After adjusting these call 'recalcFilter'.
    int filterMode{StereoSimperSVF::Mode::LP};
    float cutoff{69.0}, res{0.7};
    float cutoffMod{0.0}, resMod{0.0};

    // The internal AEG is incredibly simple. Bypass or not, and have
    // an attack and release time in seconds. These aren't modulatable
    // mostly out of laziness.
    bool ampGate{false};
    float ampAttack{0.01}, ampRelease{0.1};

    // The pre-filter VCA is unique in that it can be either internally
    // modulated and externally modulated. If ampGate is false, the internal
    // modulation is bypassed. The two vectors for modulation are a VCAMod
    // value, intended for param modulation, and a volumeNoteExpressionValue
    float preFilterVCA{1.0}, preFilterVCAMod{0.0}, volumeNoteExpressionValue{0.f};

    // Two values can modify pitch, the note expression and the bend wheel.
    // After adjusting these, call 'recalcPitch'
    float pitchNoteExpressionValue{0.f}, pitchBendWheel{0.f};

    // Finally, please set my sample rate at voice on. Thanks!
    float sampleRate{0};

    // What is my AEG state. This will advance across attack hold releasing NEWLY_OFF
    // even if the AEG is bypassed. NEWLY_OFF is a state which lets us detect voices which
    // terminate in a block so we can inform the DAW with a CLAP_EVENT_NOTE_END for polyphonic
    // voice cooperation
    enum AEGMode
    {
        OFF,
        ATTACK,
        HOLD,
        NEWLY_OFF,
        RELEASING
    } state{OFF};

    // L / R are the output.
    float L{0.f}, R{0.f};

    // start, then step the voice forever. release it on note off. sometime after that
    // the voice will transition to NEWLY_OFF which you should detect then externally
    // move it to OFF
    void start(int key);
    void step();
    void release();

    void recalcPitch();
    void recalcFilter();

    inline bool isPlaying() const { return state != OFF && state != NEWLY_OFF; }

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

  private:
    double baseFreq{440.0};
    double srInv{1.0 / 44100.0};
    float time{0}, filterTime{0};
    float releaseFrom{1.0};

    std::array<float, max_uni> panL, panR, unitShift, norm;
    std::array<double, max_uni> phase, dPhase, dPhaseInv;
};
} // namespace sst::clap_saw_demo
#endif
