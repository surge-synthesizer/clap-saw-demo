/*
 * ClapSawDemo
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

#include "saw-voice.h"
#include <cmath>
#include <algorithm>

/*
 * From a perspective of learning clap or learning how vstgui and clap work together, this file
 * can be safely skipped. It has basic DSP to generate saws into filters and a small envelope
 * state management, but it is nothing surprising.
 */

namespace sst::clap_saw_demo
{
float pival =
    3.14159265358979323846; // I always forget what you need for M_PI to work on all platforms

void SawDemoVoice::recalcPitch()
{
    baseFreq = 440.0 * pow(2.0, ((key + pitchNoteExpressionValue + pitchBendWheel +
                                  (oscDetune + oscDetuneMod) / 100) -
                                 69.0) /
                                    12.0);

    for (int i = 0; i < unison; ++i)
    {
        dPhase[i] =
            (baseFreq * pow(2.0, (uniSpread + uniSpreadMod) * unitShift[i] / 100.0 / 12.0)) /
            sampleRate;
        dPhaseInv[i] = 1.0 / dPhase[i];
    }
}

void SawDemoVoice::recalcFilter()
{
    auto co = cutoff + cutoffMod;
    auto rm = res + resMod;

    auto newfm = (StereoSimperSVF::Mode)filterMode;

    if (newfm != filter.mode)
        filter.init();
    filter.mode = newfm;
    filter.setCoeff(co, rm, srInv);
}

void SawDemoVoice::step()
{
    float AR = 1.0;

    if (state == ATTACK)
    {
        AR = time / ampAttack;
        releaseFrom = AR;
        time += srInv;
        if (time >= ampAttack)
        {
            state = HOLD;
        }

        if (ampGate)
            AR = 1.0;
    }
    else if (state == RELEASING)
    {
        auto tn = time / ampRelease;
        auto tf = (1.0 - tn);
        AR = releaseFrom * tf;
        time += srInv;
        if (time >= ampRelease)
        {
            state = NEWLY_OFF;
        }

        if (ampGate)
        {
            AR = 1.0;
            const auto lastSeg = 0.02;
            if (tn > (1.0 - lastSeg))
            {
                // Avoid a click with a last 2% fade
                AR = 1 - (tn - (1.0 - lastSeg)) / lastSeg;
            }
        }
    }
    else if (state == HOLD)
    {
        AR = 1.0;
        time = 0;
        releaseFrom = 1.0;

        if (ampGate)
            AR = 1.0;
    }

    AR *= (preFilterVCA + preFilterVCAMod + volumeNoteExpressionValue);
    L = 0;
    R = 0;

    for (int i = 0; i < unison; ++i)
    {
        /*
         * Use a cubic integrated saw and second derive it at
         * each point. This is basically the math I worked
         * out for the surge modern oscillator. The cubic function
         * which gives a clean saw is phase^3 / 6 - phase / 6.
         * Evaluate it at 3 points and then differentiate it like
         * we do in Surge Modern. The waveform is the same both
         * channels.
         */
        double phaseSteps[3];
        for (int q = -2; q <= 0; ++q)
        {
            double ph = phase[i] + q * dPhase[i];
            // Our calculation assumes phase in -1,1 and this phase is
            // in 0 1 so
            ph = ph * 2 - 1;
            phaseSteps[q + 2] = (ph * ph - 1) * ph / 6.0;
        }
        // the 0.25 here is because of the phase rescaling again
        double saw = (phaseSteps[0] + phaseSteps[2] - 2 * phaseSteps[1]) * 0.25 * dPhaseInv[i] *
                     dPhaseInv[i];

        L += 0.2 * norm[i] * AR * panL[i] * saw;
        R += 0.2 * norm[i] * AR * panR[i] * saw;

        phase[i] += dPhase[i];
        if (phase[i] > 1)
            phase[i] -= 1;
    }

    filter.step(L, R);
}

void SawDemoVoice::start(int key)
{
    srInv = 1.0 / sampleRate;

    filter.init();
    this->key = key;
    state = (ampAttack > 0 ? ATTACK : HOLD);
    time = 0;

    if (unison == 1)
    {
        unitShift[0] = 0;
        panL[0] = 1;
        panR[0] = 1;
        phase[0] = 0.0;
        norm[0] = 1.0;
    }
    else
    {
        for (int i = 0; i < unison; ++i)
        {
            float dI = 1.0 * i / (unison - 1);
            unitShift[i] = 2 * dI - 1;
            phase[i] = dI;
            panL[i] = std::cos(0.5 * pival * dI);
            panR[i] = std::sin(0.5 * pival * dI);

            norm[i] = 1.0 / sqrt(unison);
        }
    }

    recalcPitch();
    recalcFilter();
}

void SawDemoVoice::release()
{
    state = RELEASING;
    time = 0;

    if (time >= ampRelease)
        state = NEWLY_OFF;
}

void SawDemoVoice::StereoSimperSVF::setCoeff(float key, float res, float srInv)
{
    auto co = 440.0 * pow(2.0, (key - 69.0) / 12);
    co = std::clamp(co, 10.0, 15000.0); // just to be safe/lazy
    res = std::clamp(res, 0.01f, 0.99f);
    g = std::tan(pival * co * srInv);
    k = 2.0 - 2.0 * res;
    gk = g + k;
    a1 = 1.0 / (1.0 + g * gk);
    a2 = g * a1;
    a3 = g * a2;
    ak = gk * a1;
}

void SawDemoVoice::StereoSimperSVF::step(float &L, float &R)
{
    float vin[2]{L, R};
    float res[2]{0, 0};
    for (int c = 0; c < 2; ++c)
    {
        auto v3 = vin[c] - ic2eq[c];
        auto v0 = a1 * v3 - ak * ic1eq[c];
        auto v1 = a2 * v3 + a1 * ic1eq[c];
        auto v2 = a3 * v3 + a2 * ic1eq[c] + ic2eq[c];

        ic1eq[c] = 2 * v1 - ic1eq[c];
        ic2eq[c] = 2 * v2 - ic2eq[c];

        // I know that a branch in this loop is inefficient and so on
        // remember this is mostly showing you how it hangs together.
        switch (mode)
        {
        case LP:
            res[c] = v2;
            break;
        case BP:
            res[c] = v1;
            break;
        case HP:
            res[c] = v0;
            break;
        case NOTCH:
            res[c] = v2 + v0; // low + high
            break;
        case PEAK:
            res[c] = v2 - v0; // low - high;
            break;
        case ALL:
            res[c] = v2 + v0 - k * v1; // low + high - k * band
            break;
        }
    }

    L = res[0];
    R = res[1];
}

void SawDemoVoice::StereoSimperSVF::init()
{
    for (int c = 0; c < 2; ++c)
    {
        ic1eq[c] = 0.f;
        ic2eq[c] = 0.f;
    }
}
} // namespace sst::clap_saw_demo