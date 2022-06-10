/*
 * StupiSaw is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#include "stupisaw.h"
#include <iostream>
#include <cmath>
#include <cstring>

// Eject the core symbols for the plugin
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>

namespace BaconPaul
{

StupiSaw::StupiSaw(const clap_host *host)
    : clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                            clap::helpers::CheckingLevel::Maximal>(&desc, host)
{
    std::cout << "Creating BaconPaul::StupiSaw" << std::endl;
    paramToValue[pmUnisonCount] = &unisonCount;
    paramToValue[pmUnisonSpread] = &unisonSpread;
    paramToValue[pmAmpAttack] = &ampAttack;
    paramToValue[pmAmpRelease] = &ampRelease;
    paramToValue[pmCutoff] = &cutoff;
    paramToValue[pmResonance] = &resonance;
    paramToValue[pmFilterDecay] = &filterDecay;
    paramToValue[pmFilterModDepth] = &filterModDepth;
}

StupiSaw::~StupiSaw() = default;

const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr};
clap_plugin_descriptor StupiSaw::desc = {CLAP_VERSION,
                                         "org.baconpaul.stupisaw",
                                         "StupiSaw",
                                         "BaconPaul",
                                         "https://baconpaul.org",
                                         "https://baconpaul.org/no-manual",
                                         "https://baconpaul.org/no-support",
                                         "0.9.0",
                                         "It's called StupiSaw. What do you expect?",
                                         features};

/*
 * Set up a simple stereo output
 */
uint32_t StupiSaw::audioPortsCount(bool isInput) const noexcept { return isInput ? 0 : 1; }
bool StupiSaw::audioPortsInfo(uint32_t index, bool isInput,
                              clap_audio_port_info *info) const noexcept
{
    if (isInput || index != 0)
        return false;

    info->id = 0;
    strncpy(info->name, "main", sizeof(info->name));
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
    return true;
}

/*
 * On activation distribute samplerate to the voices
 */
bool StupiSaw::activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept
{
    for (auto &v : voices)
        v.sampleRate = sampleRate;
    return true;
}

/*
 * Parameter Handling is mostly validating IDs (via their inclusion in the
 * param map and setting the info and getting values from the map pointers.
 */
bool StupiSaw::implementsParams() const noexcept { return true; }
uint32_t StupiSaw::paramsCount() const noexcept { return nParams; }
bool StupiSaw::isValidParamId(clap_id paramId) const noexcept
{
    return paramToValue.find(paramId) != paramToValue.end();
}
bool StupiSaw::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept
{
    if (paramIndex >= nParams)
        return false;

    info->flags = CLAP_PARAM_IS_AUTOMATABLE;

    switch (paramIndex)
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
        info->max_value = 60;
        info->default_value = 30.0;
        break;
    }
    return true;
}
bool StupiSaw::paramsValue(clap_id paramId, double *value) noexcept
{
    *value = *paramToValue[paramId];
    return true;
}

bool StupiSaw::notePortsInfo(uint32_t index, bool isInput, clap_note_port_info *info) const noexcept
{
    if (isInput)
    {
        info->id = 1;
        info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        strncpy(info->name, "NoteInput", CLAP_NAME_SIZE);
        return true;
    }
    return false;
}

/*
 * Process is a simple algo
 *
 * 1. Read input events and update voice state and parameter state
 * 2. For each running voice, merge it onto the output
 *
 * In this implementation I read all the parameters at the top of the block
 * even though they have a time parameter which would let me interweave them with
 * my DSP in this implementation.
 */
clap_process_status StupiSaw::process(const clap_process *process) noexcept
{
    auto ev = process->in_events;
    auto sz = ev->size(ev);

    if (sz != 0)
    {
        for (auto i = 0; i < sz; ++i)
        {
            auto evt = ev->get(ev, i);

            switch (evt->type)
            {
            case CLAP_EVENT_NOTE_ON:
            {
                auto nevt = reinterpret_cast<const clap_event_note *>(evt);
                auto n = nevt->key;

                for (auto &v : voices)
                {
                    if (v.state == StupiVoice::OFF)
                    {
                        v.unison = std::max(1, std::min(7, (int)unisonCount));
                        v.ampAttack = ampAttack;
                        v.ampRelease = ampRelease;
                        v.filterDecay = filterDecay;
                        v.filterModDepth = filterModDepth;
                        v.start(n);
                        break;
                    }
                }
#if HAS_GUI
                auto r = ToUI{.type = ToUI::MIDI_NOTE_ON, .id = (uint32_t)n};
                toUiQ.try_enqueue(r);
#endif
            }
            break;
            case CLAP_EVENT_NOTE_OFF:
            {
                auto nevt = reinterpret_cast<const clap_event_note *>(evt);
                auto n = nevt->key;

                for (auto &v : voices)
                {
                    if (v.state != StupiVoice::OFF && v.key == n)
                    {
                        v.release();
                    }
                }
#if HAS_GUI
                auto r = ToUI{.type = ToUI::MIDI_NOTE_OFF, .id = (uint32_t)n};
                toUiQ.try_enqueue(r);
#endif
            }
            break;
            case CLAP_EVENT_PARAM_VALUE:
            {
                auto v = reinterpret_cast<const clap_event_param_value *>(evt);

                *paramToValue[v->param_id] = v->value;
#if HAS_GUI
                auto r =
                    ToUI{.type = ToUI::PARAM_VALUE, .id = v->param_id, .value = (double)v->value};
                toUiQ.try_enqueue(r);
#endif
            }
            break;
            }
        }
    }

#if HAS_GUI
    // Now handle any messages from the UI
    StupiSaw::FromUI r;
    while (fromUiQ.try_dequeue(r))
    {
        // So set my value
        *paramToValue[r.id] = r.value;

        // But we also need to generate outbound message to the host
        auto ov = process->out_events;
        auto evt = clap_event_param_value();
        evt.header.size = sizeof(clap_event_param_value);
        evt.header.type = (uint16_t)CLAP_EVENT_PARAM_VALUE;
        evt.header.time = 0; // for now
        evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        evt.header.flags = 0;
        evt.param_id = r.id;
        evt.value = r.value;

        ov->try_push(ov, &(evt.header));
    }
#endif

    for (auto &v : voices)
    {
        if (v.state != StupiVoice::OFF)
        {
            v.uniSpread = unisonSpread;
            v.cutoff = cutoff;
            v.res = resonance;
            v.recalcRates();
        }
    }

    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_CONTINUE;

    float **out = process->audio_outputs[0].data32;
    auto chans = process->audio_outputs->channel_count;

    for (int i = 0; i < process->frames_count; ++i)
    {
        for (int ch = 0; ch < chans; ++ch)
        {
            out[ch][i] = 0.f;
        }
        for (auto &v : voices)
        {
            if (v.state != StupiVoice::OFF)
            {
                v.step();
                if (chans >= 2)
                {
                    out[0][i] += v.L;
                    out[1][i] += v.R;
                }
                else if (chans == 1)
                {
                    out[0][i] += (v.L + v.R) * 0.5;
                }
            }
        }
    }
    return CLAP_PROCESS_CONTINUE;
}

} // namespace BaconPaul
