//
// Created by Paul Walker on 7/17/21.
//

#ifndef STUPISAW_STUPIVOICE_H
#define STUPISAW_STUPIVOICE_H

namespace BaconPaul
{
struct StupiVoice
{
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

    float baseFreq{440.0};
    float dPhase{440.0/44100.0};
    float phase{0};
    float sampleRate{0};
};
}
#endif // STUPISAW_STUPIVOICE_H
