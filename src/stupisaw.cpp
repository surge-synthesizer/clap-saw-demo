//
// Created by Paul Walker on 7/17/21.
//

#include "stupisaw.h"
#include <iostream>
#include <cmath>
#include <cstring>

namespace BaconPaul
{

StupiSaw::StupiSaw(const clap_host *host)
    : clap::Plugin(&desc, host)
{
    std::cout << "Creating a stupisaw" << std::endl;
}

clap_plugin_descriptor StupiSaw::desc = {
    CLAP_VERSION,
    "org.baconpaul.stupisaw",
    "StupiSaw",
    "BaconPaul",
    "https://baconpaul.org",
    "https://baconpaul.org/no-manual",
    "https://baconpaul.org/no-support",
    "0.9.0",
    "It's called StupiSaw. What do you expect?",
    "aliasing;example-code;dont-use",
    CLAP_PLUGIN_INSTRUMENT
};


bool StupiSaw::implementsParams() const noexcept { return true; }
uint32_t StupiSaw::paramsCount() const noexcept { return nParams; }
bool StupiSaw::isValidParamId(clap_id paramId) const noexcept
{
    // This is obviously a bit odd
    auto res = (paramId == pmUnisonCount || paramId == pmUnisonSpread || paramId == pmCutoff ||
                paramId == pmResonance || paramId == pmAmpRelease || paramId == pmAmpAttack
                || paramId == pmFilterModDepth || paramId == pmFilterDecay);
    return res;
}
bool StupiSaw::paramsInfo(int32_t paramIndex, clap_param_info *info) const noexcept
{
    if (paramIndex >= nParams) return false;

    switch( paramIndex )
    {
    case 0:
        info->id = pmUnisonCount;
        strncpy(info->name, "Unison Count", CLAP_NAME_SIZE);
        strncpy(info->module, "Oscillator", CLAP_NAME_SIZE);
        info->min_value = 1;
        info->max_value = StupiVoice::max_uni;
        info->default_value = 3;
        info->flags |= CLAP_PARAM_IS_STEPPED;
        break;
    case 1:
        info->id = pmUnisonSpread;
        strncpy(info->name, "Unison Spread in Cents", CLAP_NAME_SIZE);
        strncpy(info->module, "Oscillator", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 100;
        info->default_value = 10;
        break;
    case 2:
        info->id = pmAmpAttack;
        strncpy(info->name, "Amplitude Attack (s)", CLAP_NAME_SIZE);
        strncpy(info->module, "Oscillator", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 0.01;
        break;
    case 3:
        info->id = pmAmpRelease;
        strncpy(info->name, "Amplitude Release (s)", CLAP_NAME_SIZE);
        strncpy(info->module, "Oscillator", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 0.2;
        break;
    case 4:
        info->id = pmCutoff;
        strncpy(info->name, "Cutoff in Keys", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 1;
        info->max_value = 127;
        info->default_value = 69;
        break;
    case 5:
        info->id = pmResonance;
        strncpy(info->name, "Resonance", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 0.0;
        info->max_value = 1.0;
        info->default_value = 0.7;
        break;
    case 6:
        info->id = pmFilterDecay;
        strncpy(info->name, "Filter Env Delay (s)", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 0.0;
        info->max_value = 1.0;
        info->default_value = 0.2;
        break;
    case 7:
        info->id = pmFilterModDepth;
        strncpy(info->name, "Filter Mod Depth (keys)", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = -20;
        info->max_value = 20;
        info->default_value = 0.0;
        break;

    }
    return true;
}
bool StupiSaw::paramsValue(clap_id paramId, double *value) noexcept
{
    switch (paramId)
    {
    case pmUnisonCount:
        *value = unisonCount;
        break;
    case pmUnisonSpread:
        *value = unisonSpread;
        break;
    case pmCutoff:
        *value = cutoff;
        break;
    case pmResonance:
        *value = resonance;
        break;
    case pmAmpAttack:
        *value = ampAttack;
        break;
    case pmAmpRelease:
        *value = ampRelease;
        break;
    case pmFilterDecay:
        *value = filterDecay;
        break;
    case pmFilterModDepth:
        *value = filterModDepth;
        break;
    }

    return true;
}

clap_process_status StupiSaw::process(const clap_process *process) noexcept
{
    auto ev = process->in_events;
    auto sz = ev->size(ev);

    if (sz != 0)
    {
        for (auto i=0; i<sz; ++i)
        {
            auto evt = ev->get(ev, i);
            switch (evt->type)
            {
            case CLAP_EVENT_NOTE_ON:
            {
                auto n = evt->note;

                for (auto &v: voices)
                {
                    if (v.state == StupiVoice::OFF)
                    {
                        v.unison = std::max(1, std::min(7, (int)unisonCount));
                        v.ampAttack = ampAttack;
                        v.ampRelease = ampRelease;
                        v.filterDecay = filterDecay;
                        v.filterModDepth = filterModDepth;
                        v.start(n.key);
                        break;
                    }
                }
            }
                break;
            case CLAP_EVENT_NOTE_OFF:
            {
                auto n = evt->note;

                for (auto &v: voices)
                {
                    if (v.state != StupiVoice::OFF && v.key == n.key)
                    {
                        v.release();
                    }
                }
            }
                break;
            case CLAP_EVENT_PARAM_VALUE:
            {
                auto v = evt->param_value;
                switch (v.param_id)
                {
                case pmUnisonSpread:
                    unisonSpread = v.value;
                    break;
                case pmUnisonCount:
                    unisonCount = v.value;
                    break;
                case pmResonance:
                    resonance = v.value;
                    break;
                case pmCutoff:
                    cutoff = v.value;
                    break;
                case pmFilterModDepth:
                    filterModDepth = v.value;
                    break;
                case pmFilterDecay:
                    filterDecay = v.value;
                    break;
                case pmAmpAttack:
                    ampAttack = v.value;
                    break;
                case pmAmpRelease:
                    ampRelease = v.value;
                    break;
                }
            }
                break;
            }
        }
    }

    for (auto &v: voices)
    {
        if (v.state != StupiVoice::OFF )
        {
            v.uniSpread = unisonSpread;
            v.recalcRates();
        }
    }

    float **out = process->audio_outputs[0].data32;
    float rate = 440.0 / 44100 ;
    for (int i=0; i<process->frames_count; ++i)
    {
        out[0][i] = 0.f;
        out[1][i] = 0.f;
        for (auto &v : voices)
        {
            if (v.state != StupiVoice::OFF)
            {
                v.step();
                out[0][i] += v.L;
                out[1][i] += v.R;
            }
        }
    }
    return CLAP_PROCESS_CONTINUE;
}
bool StupiSaw::startProcessing() noexcept { return Plugin::startProcessing(); }
uint32_t StupiSaw::audioPortsCount(bool isInput) const noexcept
{
    return isInput ? 0 : 1;
}
bool StupiSaw::audioPortsInfo(uint32_t index, bool isInput,
                              clap_audio_port_info *info) const noexcept
{
    if (isInput || index != 0)
        return false;

    info->id = 0;
    strncpy(info->name, "main", sizeof(info->name));
    info->is_main = true;
    info->is_cv = false;
    info->sample_size = 32;
    info->in_place = true;
    info->channel_count = 2;
    info->channel_map = CLAP_CHMAP_STEREO;
    return true;
}
bool StupiSaw::activate(double sampleRate) noexcept {
    for (auto &v: voices)
        v.sampleRate = sampleRate;
    return true;
}
bool StupiSaw::implementsGuiCocoa() const noexcept {
    return true;
}
bool StupiSaw::guiCocoaAttach(void *nsView) noexcept {
    std::cout << "Attaching Cocoa GUI" << std::endl;
    return Plugin::guiCocoaAttach(nsView);
}

}
