/*
 * ClapSawDemo is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#include "clap-saw-demo.h"
#include <iostream>
#include <cmath>
#include <cstring>

// Eject the core symbols for the plugin
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>
#include <iomanip>

namespace sst::clap_saw_demo
{

ClapSawDemo::ClapSawDemo(const clap_host *host)
    : clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                            clap::helpers::CheckingLevel::Maximal>(&desc, host)
{
    _DBGCOUT << "Constructing ClapSawDemo" << std::endl;
    paramToValue[pmUnisonCount] = &unisonCount;
    paramToValue[pmUnisonSpread] = &unisonSpread;
    paramToValue[pmAmpAttack] = &ampAttack;
    paramToValue[pmAmpRelease] = &ampRelease;
    paramToValue[pmAmpIsGate] = &ampIsGate;
    paramToValue[pmCutoff] = &cutoff;
    paramToValue[pmResonance] = &resonance;
    paramToValue[pmPreFilterVCA] = &preFilterVCA;
    paramToValue[pmFilterMode] = &filterMode;
}

ClapSawDemo::~ClapSawDemo() = default;

const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr};
clap_plugin_descriptor ClapSawDemo::desc = {CLAP_VERSION,
                                            "org.surge-synth-team.clap-saw-demo",
                                            "Clap Saw Demo Synth",
                                            "Surge Synth team",
                                            "https://surge-synth-team.org",
                                            "",
                                            "",
                                            "0.9.0",
                                            "A simple sawtooth synth to show CLAP features.",
                                            features};

/*
 * Set up a simple stereo output
 */
uint32_t ClapSawDemo::audioPortsCount(bool isInput) const noexcept { return isInput ? 0 : 1; }
bool ClapSawDemo::audioPortsInfo(uint32_t index, bool isInput,
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
bool ClapSawDemo::activate(double sampleRate, uint32_t minFrameCount,
                           uint32_t maxFrameCount) noexcept
{
    for (auto &v : voices)
        v.sampleRate = sampleRate;
    return true;
}

/*
 * Parameter Handling is mostly validating IDs (via their inclusion in the
 * param map and setting the info and getting values from the map pointers.
 */
bool ClapSawDemo::implementsParams() const noexcept { return true; }
uint32_t ClapSawDemo::paramsCount() const noexcept { return nParams; }
bool ClapSawDemo::isValidParamId(clap_id paramId) const noexcept
{
    return paramToValue.find(paramId) != paramToValue.end();
}
bool ClapSawDemo::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept
{
    if (paramIndex >= nParams)
        return false;

    info->flags = CLAP_PARAM_IS_AUTOMATABLE;

    auto mod = CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID;

    switch (paramIndex)
    {
    case 0:
        info->id = pmUnisonCount;
        strncpy(info->name, "Unison Count", CLAP_NAME_SIZE);
        strncpy(info->module, "Oscillator", CLAP_NAME_SIZE);
        info->min_value = 1;
        info->max_value = SawDemoVoice::max_uni;
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
        info->flags |= mod;
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
        info->id = pmAmpIsGate;
        strncpy(info->name, "Deactivate Amp Envelope", CLAP_NAME_SIZE);
        strncpy(info->module, "Oscillator", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 0;
        info->flags |= CLAP_PARAM_IS_STEPPED;
        break;
    case 5:
        info->id = pmPreFilterVCA;
        strncpy(info->name, "Pre Filter VCA", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 1;
        info->flags |= mod;
        break;
    case 6:
        info->id = pmCutoff;
        strncpy(info->name, "Cutoff in Keys", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 1;
        info->max_value = 127;
        info->default_value = 69;
        info->flags |= mod;
        break;
    case 7:
        info->id = pmResonance;
        strncpy(info->name, "Resonance", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 0.0;
        info->max_value = 1.0;
        info->default_value = 0.7;
        info->flags |= mod;
        break;
    case 8:
        info->id = pmFilterMode;
        strncpy(info->name, "Filter Type", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = SawDemoVoice::StereoSimperSVF::Mode::LP;
        info->max_value = SawDemoVoice::StereoSimperSVF::Mode::ALL;
        info->default_value = 0;
        info->flags |= CLAP_PARAM_IS_STEPPED;
        break;
    }
    return true;
}
bool ClapSawDemo::paramsValue(clap_id paramId, double *value) noexcept
{
    *value = *paramToValue[paramId];
    return true;
}

bool ClapSawDemo::notePortsInfo(uint32_t index, bool isInput,
                                clap_note_port_info *info) const noexcept
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
clap_process_status ClapSawDemo::process(const clap_process *process) noexcept
{
    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_CONTINUE;

    // Now handle any messages from the UI
    bool uiAdjustedValues{false};
    ClapSawDemo::FromUI r;
    while (fromUiQ.try_dequeue(r))
    {
        switch (r.type)
        {
        case FromUI::BEGIN_EDIT:
        case FromUI::END_EDIT:
        {
            auto ov = process->out_events;
            auto evt = clap_event_param_gesture();
            evt.header.size = sizeof(clap_event_param_gesture);
            evt.header.type = (r.type == FromUI::BEGIN_EDIT ? CLAP_EVENT_PARAM_GESTURE_BEGIN
                                                            : CLAP_EVENT_PARAM_GESTURE_END);
            evt.header.time = 0;
            evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            evt.header.flags = 0;
            evt.param_id = r.id;
            ov->try_push(ov, &evt.header);

            break;
        }
        case FromUI::ADJUST_VALUE:
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

            uiAdjustedValues = true;
        }
        }
    }

    if (uiAdjustedValues)
        pushParamsToVoices();

    float **out = process->audio_outputs[0].data32;
    auto chans = process->audio_outputs->channel_count;

    auto ev = process->in_events;
    auto sz = ev->size(ev);

    const clap_event_header_t  *nextEvent{nullptr};
    uint32_t nextEventIndex{0};
    if (sz != 0)
    {
        nextEvent = ev->get(ev, nextEventIndex);
    }

    for (int i = 0; i < process->frames_count; ++i)
    {
        // Do I have an event to process
        while(nextEvent && nextEvent->time == i)
        {
            handleInboundEvent(nextEvent);
            nextEventIndex++;
            if (nextEventIndex >= sz)
                nextEvent = nullptr;
            else
                nextEvent = ev->get(ev, nextEventIndex);
        }

        for (int ch = 0; ch < chans; ++ch)
        {
            out[ch][i] = 0.f;
        }
        for (auto &v : voices)
        {
            if (v.state != SawDemoVoice::OFF && v.state != SawDemoVoice::NEWLY_OFF)
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

    for (auto &v : voices)
    {
        if (v.state == SawDemoVoice::NEWLY_OFF)
        {
            auto ov = process->out_events;
            v.state = SawDemoVoice::OFF;

            auto evt = clap_event_note();
            evt.header.size = sizeof(clap_event_note);
            evt.header.type = (uint16_t)CLAP_EVENT_NOTE_END;
            evt.header.time = process->frames_count - 1;
            evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            evt.header.flags = 0;

            evt.port_index = 0;
            evt.channel = 0; // FIXME
            evt.key = v.key;
            evt.note_id = v.note_id;
            evt.velocity = 0.0;

            ov->try_push(ov, &(evt.header));

            dataCopyForUI.updateCount++;
            dataCopyForUI.polyphony--;
        }
    }

    // We should have gotten all the events
    assert(!nextEvent);
    return CLAP_PROCESS_CONTINUE;
}

void ClapSawDemo::handleInboundEvent(const clap_event_header_t *evt)
{
    switch (evt->type)
    {
    case CLAP_EVENT_MIDI:
    {
        auto mevt = reinterpret_cast<const clap_event_midi *>(evt);
        auto msg = mevt->data[0] & 0xF0;
        switch(msg)
        {
        case 0x90:
        {
            // Hosts should prefer CLAP_NOTE events but if they dont
            handleNoteOn(mevt->data[1], -1);
            break;
        }
        case 0x80:
        {
            // Hosts should prefer CLAP_NOTE events but if they dont
            handleNoteOff(mevt->data[1]);
            break;
        }
        case 0xE0:
        {
            // pitch bend
            auto bv = (mevt->data[1] + mevt->data[2] * 128 - 8192) / 8192.0;

            for (auto &v : voices)
            {
                v.pitchBendWheel = bv * 2; // just harcode a pitch bend depth of 2
                v.recalcPitch();
            }

            break;
        }
        }
        break;
    }
    case CLAP_EVENT_NOTE_ON:
    {
        auto nevt = reinterpret_cast<const clap_event_note *>(evt);
        handleNoteOn(nevt->key, nevt->note_id);
    }
    break;
    case CLAP_EVENT_NOTE_OFF:
    {
        auto nevt = reinterpret_cast<const clap_event_note *>(evt);
        handleNoteOff(nevt->key);
    }
    break;
    case CLAP_EVENT_PARAM_VALUE:
    {
        auto v = reinterpret_cast<const clap_event_param_value *>(evt);

        *paramToValue[v->param_id] = v->value;
        auto r = ToUI();
        r.type = ToUI::PARAM_VALUE;
        r.id = v->param_id;
        r.value = (double)v->value;

        toUiQ.try_enqueue(r);
        pushParamsToVoices();
    }
    case CLAP_EVENT_PARAM_MOD:
    {
        auto pevt = reinterpret_cast<const clap_event_param_mod *>(evt);
        auto pd = pevt->param_id;
        auto applyToVoice = [&pevt](auto &v)
        {
            auto pd = pevt->param_id;
            switch (pd)
            {
            case paramIds::pmCutoff:
            {
                v.cutoffMod = pevt->amount;
                v.recalcFilter();
                break;
            }
            case paramIds::pmUnisonSpread:
            {
                v.uniSpreadMod = pevt->amount;
                v.recalcPitch();
                break;
            }
            case paramIds::pmResonance:
            {
                v.resMod = pevt->amount;
                v.recalcFilter();
                break;
            }
            case paramIds::pmPreFilterVCA:
            {
                v.preFilterVCAMod = pevt->amount;
            }
            }
        };

        if (pevt->note_id >= 0)
        {
            // poly by note_id
            for (auto &v : voices)
            {
                if (v.note_id == pevt->note_id)
                {
                    applyToVoice(v);
                }
            }
        }
        else if (pevt->key >= 0)
        {
            // poly by PCK
            for (auto &v : voices)
            {
                if (v.key == pevt->key)
                {
                    applyToVoice(v);
                }
            }
        }
        else
        {
            // mono
            for (auto &v : voices)
            {
                applyToVoice(v);
            }
        }
    }
    break;
    case CLAP_EVENT_NOTE_EXPRESSION:
    {
        auto pevt = reinterpret_cast<const clap_event_note_expression *>(evt);
        for (auto &v : voices)
        {
            // Note expressions work on key not note id
            if (v.key == pevt->key)
            {
                switch (pevt->expression_id)
                {
                case CLAP_NOTE_EXPRESSION_VOLUME:
                    // I can mod the VCA
                    v.volumeNoteExpressionValue = pevt->value - 1.0;
                    break;
                case CLAP_NOTE_EXPRESSION_TUNING:
                    v.pitchNoteExpressionValue = pevt->value;
                    v.recalcPitch();
                    break;
                }
            }
        }
    }
    break;
    }
}

void ClapSawDemo::handleNoteOn(int key, int noteid)
{
    for (auto &v : voices)
    {
        if (v.state == SawDemoVoice::OFF)
        {
            v.unison = std::max(1, std::min(7, (int)unisonCount));
            v.filterMode = (int)static_cast<int>(filterMode); // I could be less lazy obvs
            v.note_id = noteid;

            v.uniSpread = unisonSpread;
            v.cutoff = cutoff;
            v.res = resonance;
            v.preFilterVCA = preFilterVCA;
            v.ampRelease = scaleTimeParamToSeconds(ampRelease);
            v.ampAttack = scaleTimeParamToSeconds(ampAttack);
            v.ampGate = ampIsGate > 0.5;


            // reset all the modulations
            v.cutoffMod = 0;
            v.resMod = 0;
            v.preFilterVCAMod = 0;
            v.uniSpreadMod = 0;
            v.volumeNoteExpressionValue = 0;
            v.pitchNoteExpressionValue = 0;

            v.start(key);
            break;
        }
    }

    dataCopyForUI.updateCount++;
    dataCopyForUI.polyphony++;
    auto r = ToUI();
    r.type = ToUI::MIDI_NOTE_ON;
    r.id = (uint32_t)key;
    toUiQ.try_enqueue(r);
}

void ClapSawDemo::handleNoteOff(int n)
{
    for (auto &v : voices)
    {
        if (v.state != SawDemoVoice::OFF && v.key == n)
        {
            v.release();
        }
    }

    auto r = ToUI();
    r.type = ToUI::MIDI_NOTE_OFF;
    r.id = (uint32_t)n;
    toUiQ.try_enqueue(r);
}

void ClapSawDemo::pushParamsToVoices()
{
    for (auto &v : voices)
    {
        if (v.state != SawDemoVoice::OFF && v.state != SawDemoVoice::NEWLY_OFF)
        {
            v.uniSpread = unisonSpread;
            v.cutoff = cutoff;
            v.res = resonance;
            v.preFilterVCA = preFilterVCA;
            v.ampRelease = scaleTimeParamToSeconds(ampRelease);
            v.ampAttack = scaleTimeParamToSeconds(ampAttack);
            v.ampGate = ampIsGate > 0.5;
            v.filterMode = filterMode;

            v.recalcPitch();
            v.recalcFilter();
        }
    }
}

bool ClapSawDemo::paramsValueToText(clap_id paramId, double value, char *display,
                                    uint32_t size) noexcept
{
    auto pid = (paramIds)paramId;
    std::string sValue{"ERROR"};
    auto n2s = [](auto n)
    {
        std::ostringstream  oss;
        oss << std::setprecision(6) << n;
        return oss.str();
    };
    switch(pid)
    {
    case pmResonance:
    case pmPreFilterVCA:
        sValue = n2s(value);
        break;
    case pmAmpRelease:
    case pmAmpAttack:
        sValue = n2s(scaleTimeParamToSeconds(value)) + " s";
        break;
    case pmUnisonCount:
        sValue = n2s(static_cast<int>(value)) + " voices";
        break;
    case pmUnisonSpread:
        sValue = n2s(value) + " cents";
        break;
    case pmAmpIsGate:
        sValue = value > 0.5 ? "AEG Bypassed" : "AEG On";
        break;
    case pmCutoff:
    {
        auto co = 440 * pow(2.0, (value - 69)/12);
        sValue = n2s(co) + " Hz";
        break;
    }
    case pmFilterMode:
    {
        auto fm = (SawDemoVoice::StereoSimperSVF::Mode)static_cast<int>(value);
        switch(fm)
        {
        case SawDemoVoice::StereoSimperSVF::LP:
            sValue = "LowPass";
            break;
        case SawDemoVoice::StereoSimperSVF::BP:
            sValue = "BandPass";
            break;
        case SawDemoVoice::StereoSimperSVF::HP:
            sValue = "HighPass";
            break;
        case SawDemoVoice::StereoSimperSVF::NOTCH:
            sValue = "Notch";
            break;
        case SawDemoVoice::StereoSimperSVF::PEAK:
            sValue = "Peak";
            break;
        case SawDemoVoice::StereoSimperSVF::ALL:
            sValue = "AllPass";
            break;
        }
        break;
    }

    }

    strncpy(display, sValue.c_str(), CLAP_NAME_SIZE);
    return true;
}

float ClapSawDemo::scaleTimeParamToSeconds(float param)
{
    auto scaleTime = std::clamp((param - 2.0 / 3.0) * 6, -100.0, 2.0);
    auto res = powf(2.f, scaleTime);
    return res;

}

bool ClapSawDemo::stateSave(const clap_ostream *stream) noexcept
{
    // Oh this is soooo bad. Please don't judge me. I'm just trying to get this
    // together for launch day! If you are using this as an example for your plugins,
    // you should write a less dumb serializer of course. On and I bet this might have
    // a locale problem?
    std::ostringstream oss;
    oss << "STREAM-VERSION-1;";
    for (const auto &[id, val] : paramToValue)
    {
        oss << id << "=" << *val << ";";
    }
    _DBGCOUT << oss.str() << std::endl;

    auto st = oss.str();
    auto c = st.c_str();
    auto s = st.length() + 1; // write the null terminator
    while (s>0)
    {
        auto r = stream->write(stream, c, s);
        if (r < 0)
            return false;
        s -= r;
        c += r;
    }
    return true;
}


bool ClapSawDemo::stateLoad(const clap_istream *stream) noexcept
{
    // Again, see the comment above on 'this is terrible'
    char buffer[4096];
    char *bp = &(buffer[0]);
    int64_t rd;
    while ((rd = stream->read(stream, bp, 256)) > 0)
    {
        bp += rd;
    }

    auto dat = std::string(buffer);
    _DBGCOUT << dat << std::endl;

    std::vector<std::string> items;
    size_t spos{0};
    while ((spos = dat.find(";")) != std::string::npos)
    {
        auto l = dat.substr(0, spos);
        dat = dat.substr(spos+1);
        items.push_back(l);
    }

    if (items[0] != "STREAM-VERSION-1")
    {
        _DBGCOUT << "Invalid stream" << std::endl;
        return false;
    }
    for (auto i : items)
    {
        auto epos = i.find( "=" );
        if (epos == std::string::npos)
            continue; // oh well
        auto id = std::atoi(i.substr(0, epos).c_str());
        auto val = std::atof(i.substr(epos+1).c_str());

        *(paramToValue[(paramIds)id]) = val;
    }

    pushParamsToVoices();
    return true;
}

#if IS_LINUX
bool ClapSawDemo::registerTimer(uint32_t interv, clap_id *id)
{
    return _host.timerSupportRegister(interv, id);
}
bool ClapSawDemo::unregisterTimer(clap_id id) { return _host.timerSupportUnregister(id); }
bool ClapSawDemo::registerPosixFd(int fd)
{
    return _host.posixFdSupportRegister(fd, CLAP_POSIX_FD_READ | CLAP_POSIX_FD_WRITE |
                                                CLAP_POSIX_FD_ERROR);
}
bool ClapSawDemo::unregisterPosixFD(int fd) { return _host.posixFdSupportUnregister(fd); }
#endif

} // namespace sst::clap_saw_demo
