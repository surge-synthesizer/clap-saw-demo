//
// Created by Paul Walker on 7/17/21.
//

#include "stupivoice.h"
#include <cmath>
#include <iostream>

namespace BaconPaul
{
void StupiVoice::recalcRates()
{
    for (int i=0; i<unison; ++i)
    {
        dPhase[i] = (baseFreq * pow( 2.0, uniSpread * unitShift[i]/100.0 / 12.0) ) / sampleRate;
    }
    srInv = 1.0 / sampleRate;
}

void StupiVoice::step()
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
    }
    else if (state == RELEASE)
    {
        AR = releaseFrom * (1.0 - time / ampRelease);
        time += srInv;
        if (time >= ampRelease)
        {
            state = OFF;
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
    for (int i=0; i<unison; ++i)
    {
        L += 0.2 * norm[i] * AR * panL[i] * (phase[i] * 2 - 1);
        R += 0.2 * norm[i] * AR * panR[i] * (phase[i] * 2 - 1);

        phase[i] += dPhase[i];
        if (phase[i] > 1)
            phase[i] -= 1;
    }
}

void StupiVoice::start(int key)
{
    this->key = key;
    baseFreq = 440.0 * pow( 2.0, (key-69.0)/12.0);
    state = ATTACK;
    time = 0;

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
            float dI = 1.0 * i / (unison-1);
            unitShift[i] = 2 * dI - 1;
            phase[i] = dI;
            panL[i] = std::cos(0.5 * M_PI * dI);
            panR[i] = std::sin(0.5 * M_PI * dI);

            norm[i] = 1.0 / sqrt(unison);
        }
    }

    recalcRates();
}

void StupiVoice::release()
{
    state = RELEASE;
    time = 0;
}
}