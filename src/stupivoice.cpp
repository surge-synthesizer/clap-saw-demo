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
    dPhase = baseFreq / sampleRate;
}

void StupiVoice::step()
{
    L = 0.2 * ( phase * 2 - 1 );
    R = 0.2 * ( phase * 2 - 1 );

    phase += dPhase;
    if (phase > 1)
        phase -= 1;
}

void StupiVoice::start(int key)
{
    std::cout << "Starting" << std::endl;
    this->key = key;
    baseFreq = 440.0 * pow( 2.0, (key-69.0)/12.0);
    state = HOLD;
    recalcRates();
}

void StupiVoice::release()
{
    state = OFF;
}
}