/*
 * StupiSaw is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#include "stupivoice.h"
#include <cmath>

/*
 * Nothing here is really clap related at all. It just makes some voices.
 */
namespace BaconPaul
{
void StupiVoice::recalcRates()
{
    for (int i = 0; i < unison; ++i)
    {
        dPhase[i] = (baseFreq * pow(2.0, uniSpread * unitShift[i] / 100.0 / 12.0)) / sampleRate;
    }
    srInv = 1.0 / sampleRate;
}

void StupiVoice::step()
{
    auto co = cutoff + cutoffMod;
    filter.setCoeff(co, res, srInv);
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
    }
    else if (state == RELEASE)
    {
        AR = releaseFrom * (1.0 - time / ampRelease);
        time += srInv;
        if (time >= ampRelease)
        {
            state = NEWLY_OFF;
        }
    }
    else if (state == HOLD)
    {
        AR = 1.0;
        time = 0;
        releaseFrom = 1.0;
    }

    L = 0;
    R = 0;
    for (int i = 0; i < unison; ++i)
    {
        L += 0.2 * norm[i] * AR * panL[i] * (phase[i] * 2 - 1);
        R += 0.2 * norm[i] * AR * panR[i] * (phase[i] * 2 - 1);

        phase[i] += dPhase[i];
        if (phase[i] > 1)
            phase[i] -= 1;
    }

    filter.step(L, R);
}

void StupiVoice::start(int key)
{
    filter.init();
    this->key = key;
    baseFreq = 440.0 * pow(2.0, (key - 69.0) / 12.0);
    state = ATTACK;
    filterState = DECAY;
    time = 0;
    filterTime = 0;

    if (unison == 1)
    {
        unitShift[0] = 0;
        panL[0] = 1.0;
        panR[0] = 1.0;
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
            panL[i] = std::cos(0.5 * M_PI * dI);
            panR[i] = std::sin(0.5 * M_PI * dI);

            norm[i] = 1.0 / sqrt(unison);
        }
    }

    recalcRates();
    filter.setCoeff(cutoff, res, srInv);
}

void StupiVoice::release()
{
    state = RELEASE;
    time = 0;
}

void StupiVoice::StereoBiQuadLPF::setCoeff(float key, float res, float srInv)
{
    auto freq = 440.0 * pow(2.0, (key - 69) / 12.0);
    auto w0 = 2.0 * M_PI * freq * srInv;
    auto cw0 = std::cos(w0);
    auto sw0 = std::sin(w0);
    auto alp = sw0 / (2.0 * res);

    b0 = (1.0 - cw0) / 2;
    b1 = 1.0 - cw0;
    b2 = b0;
    a0 = 1 + alp;
    a1 = -2 * cw0;
    a2 = 1 - alp;
}

void StupiVoice::StereoBiQuadLPF::step(float &L, float &R)
{
    x[0][2] = x[0][1];
    x[0][1] = x[0][0];
    x[0][0] = L;
    x[1][2] = x[1][1];
    x[1][1] = x[1][0];
    x[1][0] = R;

    y[0][2] = y[0][1];
    y[0][1] = y[0][0];

    y[1][2] = y[1][1];
    y[1][1] = y[1][0];

    for (int c = 0; c < 2; ++c)
    {
        y[c][0] = (b0 / a0) * x[c][0] + (b1 / a0) * x[c][1] + (b2 / a0) * x[c][2] -
                  (a1 / a0) * y[c][1] - (a2 / a0) * y[c][2];
    }
    L = y[0][0];
    R = y[1][0];
}

void StupiVoice::StereoBiQuadLPF::init()
{
    for (int c = 0; c < 2; ++c)
        for (int s = 0; s < 3; ++s)
        {
            x[c][s] = 0;
            y[c][s] = 0;
        }
}
} // namespace BaconPaul