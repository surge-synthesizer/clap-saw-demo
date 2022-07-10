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
#include <locale>

namespace sst::clap_saw_demo
{

ClapSawDemo::ClapSawDemo(const clap_host *host)
    : clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                            clap::helpers::CheckingLevel::Maximal>(&desc, host)
{
    _DBGCOUT << "Constructing ClapSawDemo" << std::endl;
    paramToValue[pmUnisonCount] = &unisonCount;
    paramToValue[pmUnisonSpread] = &unisonSpread;
    paramToValue[pmOscDetune] = &oscDetune;
    paramToValue[pmAmpAttack] = &ampAttack;
    paramToValue[pmAmpRelease] = &ampRelease;
    paramToValue[pmAmpIsGate] = &ampIsGate;
    paramToValue[pmCutoff] = &cutoff;
    paramToValue[pmResonance] = &resonance;
    paramToValue[pmPreFilterVCA] = &preFilterVCA;
    paramToValue[pmFilterMode] = &filterMode;

    terminatedVoices.reserve(max_voices * 4);
}
ClapSawDemo::~ClapSawDemo()
{
    // I *think* this is a bitwig bug that they won't call guiDestroy if destroying a plugin
    // with an open window but
    if (editor)
        guiDestroy();
}

const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr};
clap_plugin_descriptor ClapSawDemo::desc = {CLAP_VERSION,
                                            "org.surge-synth-team.clap-saw-demo",
                                            "Clap Saw Demo Synth",
                                            "Surge Synth Team",
                                            "https://surge-synth-team.org",
                                            "",
                                            "",
                                            "1.0.0",
                                            "A simple sawtooth synth to show CLAP features.",
                                            features};
/*
 * PARAMETER SETUP SECTION
 */
bool ClapSawDemo::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept
{
    if (paramIndex >= nParams)
        return false;

    /*
     * Our job is to populate the clap_param_info. We set each of our parameters as AUTOMATABLE
     * and then begin setting per-parameter features.
     */
    info->flags = CLAP_PARAM_IS_AUTOMATABLE;

    /*
     * These constants activate polyphonic modulatability on a parameter. Not all the params here
     * support that
     */
    auto mod = CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID |
               CLAP_PARAM_IS_MODULATABLE_PER_KEY;

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
        info->id = pmOscDetune;
        strncpy(info->name, "Oscillator Detuning (in cents)", CLAP_NAME_SIZE);
        strncpy(info->module, "Oscillator", CLAP_NAME_SIZE);
        info->min_value = -200;
        info->max_value = 200;
        info->default_value = 0;
        info->flags |= mod;
        break;
    case 3:
        info->id = pmAmpAttack;
        strncpy(info->name, "Amplitude Attack (s)", CLAP_NAME_SIZE);
        strncpy(info->module, "Amplitude Envelope Generator", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 0.01;
        break;
    case 4:
        info->id = pmAmpRelease;
        strncpy(info->name, "Amplitude Release (s)", CLAP_NAME_SIZE);
        strncpy(info->module, "Amplitude Envelope Generator", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 0.2;
        break;
    case 5:
        info->id = pmAmpIsGate;
        strncpy(info->name, "Deactivate Amp Envelope", CLAP_NAME_SIZE);
        strncpy(info->module, "Amplitude Envelope Generator", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 0;
        info->flags |= CLAP_PARAM_IS_STEPPED;
        break;
    case 6:
        info->id = pmPreFilterVCA;
        strncpy(info->name, "Pre Filter VCA", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = 1;
        info->flags |= mod;
        break;
    case 7:
        info->id = pmCutoff;
        strncpy(info->name, "Cutoff in Keys", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 1;
        info->max_value = 127;
        info->default_value = 69;
        info->flags |= mod;
        break;
    case 8:
        info->id = pmResonance;
        strncpy(info->name, "Resonance", CLAP_NAME_SIZE);
        strncpy(info->module, "Filter", CLAP_NAME_SIZE);
        info->min_value = 0.0;
        info->max_value = 1.0;
        info->default_value = 0.7;
        info->flags |= mod;
        break;
    case 9:
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

bool ClapSawDemo::paramsValueToText(clap_id paramId, double value, char *display,
                                    uint32_t size) noexcept
{
    auto pid = (paramIds)paramId;
    std::string sValue{"ERROR"};
    auto n2s = [](auto n)
    {
        std::ostringstream oss;
        oss << std::setprecision(6) << n;
        return oss.str();
    };
    switch (pid)
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
    {
        int vc = static_cast<int>(value);
        sValue = n2s(vc) + (vc == 1 ? " voice" : " voices");
        break;
    }
    case pmUnisonSpread:
    case pmOscDetune:
        sValue = n2s(value) + " cents";
        break;
    case pmAmpIsGate:
        sValue = value > 0.5 ? "AEG Bypassed" : "AEG On";
        break;
    case pmCutoff:
    {
        auto co = 440 * pow(2.0, (value - 69) / 12);
        sValue = n2s(co) + " Hz";
        break;
    }
    case pmFilterMode:
    {
        auto fm = (SawDemoVoice::StereoSimperSVF::Mode) static_cast<int>(value);
        switch (fm)
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

/*
 * Stereo out, Midi in, in a pretty obvious way.
 * The only trick is the idi in also has NOTE_DIALECT_CLAP which provides us
 * with options on note expression and the like.
 */
bool ClapSawDemo::audioPortsInfo(uint32_t index, bool isInput,
                                 clap_audio_port_info *info) const noexcept
{
    if (isInput || index != 0)
        return false;

    info->id = 0;
    info->in_place_pair = CLAP_INVALID_ID;
    strncpy(info->name, "main", sizeof(info->name));
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
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
 * The process function is the heart of any CLAP. It reads inbound events,
 * generates audio if appropriate, writes outbound events, and informs the host
 * to continue operating.
 *
 * In the ClapSawDemo, our process loop has 3 basic stages
 *
 * 1. See if the UI has sent us any events on the thread-safe UI Queue (
 *    see the discussion in the clap header file for this structure), apply them
 *    to my internal state, and generate CLAP changed messages
 *
 * 2. Iterate over samples rendering the voices, and if an inbound event is coincident
 *    with a sample, process that event for note on, modulation, parameter automation, and so on
 *
 * 3. Detect any voices which have terminated in the block (their state has become 'NEWLY_OFF'),
 *    update them to 'OFF' and send a CLAP NOTE_END event to terminate any polyphonic modulators.
 */
clap_process_status ClapSawDemo::process(const clap_process *process) noexcept
{
    // If I have no outputs, do nothing
    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_CONTINUE;

    /*
     * Stage 1:
     *
     * The UI can send us gesture begin/end events which translate in to a
     * `clap_event_param_gesture` or value adjustments.
     */
    handleEventsFromUIQueue(process->out_events);

    /*
     * Stage 2: Create the AUDIO output and process events
     *
     * CLAP has a single inbound event loop where every event is time stamped with
     * a sample id. This means the process loop can easily interleave note and parameter
     * and other events with audio generation. Here we do everything completely sample accurately
     * by maintaining a pointer to the 'nextEvent' which we check at every sample.
     */
    float **out = process->audio_outputs[0].data32;
    auto chans = process->audio_outputs->channel_count;

    auto ev = process->in_events;
    auto sz = ev->size(ev);

    // This pointer is the sentinel to our next event which we advance once an event is processed
    const clap_event_header_t *nextEvent{nullptr};
    uint32_t nextEventIndex{0};
    if (sz != 0)
    {
        nextEvent = ev->get(ev, nextEventIndex);
    }

    for (int i = 0; i < process->frames_count; ++i)
    {
        // Do I have an event to process. Note that multiple events
        // can occur on the same sample, hence 'while' not 'if'
        while (nextEvent && nextEvent->time == i)
        {
            // handleInboundEvent is a separate function which adjusts the state based
            // on event type. We segregate it for clarity but you really should read it!
            handleInboundEvent(nextEvent);
            nextEventIndex++;
            if (nextEventIndex >= sz)
                nextEvent = nullptr;
            else
                nextEvent = ev->get(ev, nextEventIndex);
        }

        // This is a simple accumulator of output across our active voices.
        // See saw-voice.h for information on the individual voice.
        for (int ch = 0; ch < chans; ++ch)
        {
            out[ch][i] = 0.f;
        }
        for (auto &v : voices)
        {
            if (v.isPlaying())
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

    /*
     * Stage 3 is to inform the host of our terminated voices.
     *
     * This allows hosts which support polyphonic modulation to terminate those
     * modulators, and it is also the reason we have the NEWLY_OFF state in addition
     * to the OFF state.
     *
     * Note that there are two ways to enter the terminatedVoices array. The first
     * is here through natural state transition to NEWLY_OFF and the second is in
     * handleNoteOn when we steal a voice.
     */
    for (auto &v : voices)
    {
        if (v.state == SawDemoVoice::NEWLY_OFF)
        {
            terminatedVoices.emplace_back(v.portid, v.channel, v.key, v.note_id);
            v.state = SawDemoVoice::OFF;
        }
    }

    for (const auto &[portid, channel, key, note_id] : terminatedVoices)
    {
        auto ov = process->out_events;
        auto evt = clap_event_note();
        evt.header.size = sizeof(clap_event_note);
        evt.header.type = (uint16_t)CLAP_EVENT_NOTE_END;
        evt.header.time = process->frames_count - 1;
        evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        evt.header.flags = 0;

        evt.port_index = portid;
        evt.channel = channel;
        evt.key = key;
        evt.note_id = note_id;
        evt.velocity = 0.0;

        ov->try_push(ov, &(evt.header));

        dataCopyForUI.updateCount++;
        dataCopyForUI.polyphony--;
    }
    terminatedVoices.clear();

    // We should have gotten all the events
    assert(!nextEvent);
    return CLAP_PROCESS_CONTINUE;
}

/*
 * handleInboundEvent provides the core event mechanism including
 * voice activation and deactivation, parameter modulation, note expression,
 * and so on.
 *
 * It reads, unsurprisingly, as a simple switch over type with reactions.
 */
void ClapSawDemo::handleInboundEvent(const clap_event_header_t *evt)
{
    if (evt->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    switch (evt->type)
    {
    case CLAP_EVENT_MIDI:
    {
        /*
         * We advertise both CLAP_DIALECT_MIDI and CLAP_DIALECT_CLAP_NOTE so we do need
         * to handle midi events. CLAP just gives us MIDI 1 (or 2 if you want, but I didn't code
         * that) streams to do with as you wish. The CLAP_MIDI_EVENT here does the obvious thing.
         */
        auto mevt = reinterpret_cast<const clap_event_midi *>(evt);
        auto msg = mevt->data[0] & 0xF0;
        auto chan = mevt->data[0] & 0x0F;
        switch (msg)
        {
        case 0x90:
        {
            // Hosts should prefer CLAP_NOTE events but if they don't
            handleNoteOn(mevt->port_index, chan, mevt->data[1], -1);
            break;
        }
        case 0x80:
        {
            // Hosts should prefer CLAP_NOTE events but if they don't
            handleNoteOff(mevt->port_index, chan, mevt->data[1]);
            break;
        }
        case 0xE0:
        {
            // pitch bend
            auto bv = (mevt->data[1] + mevt->data[2] * 128 - 8192) / 8192.0;

            for (auto &v : voices)
            {
                v.pitchBendWheel = bv * 2; // just hardcode a pitch bend depth of 2
                v.recalcPitch();
            }

            break;
        }
        }
        break;
    }
    /*
     * CLAP_EVENT_NOTE_ON and OFF simply deliver the event to the note creators below,
     * which find (probably) and activate a spare or playing voice. Our 'voice stealing'
     * algorithm here is 'just don't play a note 65 if 64 are ringing. Remember this is an
     * example synth!
     */
    case CLAP_EVENT_NOTE_ON:
    {
        auto nevt = reinterpret_cast<const clap_event_note *>(evt);
        handleNoteOn(nevt->port_index, nevt->channel, nevt->key, nevt->note_id);
    }
    break;
    case CLAP_EVENT_NOTE_OFF:
    {
        auto nevt = reinterpret_cast<const clap_event_note *>(evt);
        handleNoteOff(nevt->port_index, nevt->channel, nevt->key);
    }
    break;
    /*
     * CLAP_EVENT_PARAM_VALUE sets a value. What happens if you change a parameter
     * outside a modulation context. We simply update our engine value and, if an editor
     * is attached, send an editor message.
     */
    case CLAP_EVENT_PARAM_VALUE:
    {
        auto v = reinterpret_cast<const clap_event_param_value *>(evt);

        *paramToValue[v->param_id] = v->value;
        pushParamsToVoices();

        if (editor)
        {
            auto r = ToUI();
            r.type = ToUI::PARAM_VALUE;
            r.id = v->param_id;
            r.value = (double)v->value;

            toUiQ.try_enqueue(r);
        }
    }
    break;
    /*
     * CLAP_EVENT_PARAM_MOD provides both monophonic and polyphonic modulation.
     * We do this by seeing which parameter is modulated then adjusting the
     * side-by-side modulation values in a voice.
     */
    case CLAP_EVENT_PARAM_MOD:
    {
        auto pevt = reinterpret_cast<const clap_event_param_mod *>(evt);

        // This little lambda updates a modulation slot in a voice properly
        auto applyToVoice = [&pevt](auto &v)
        {
            if (!v.isPlaying())
                return;

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
            case paramIds::pmOscDetune:
            {
                // _DBGCOUT << "Detune Mod" << _D(pevt->amount) << std::endl;
                v.oscDetuneMod = pevt->amount;
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

        /*
         * The real meat is here. If we have a note id, find the note and modulate it.
         * Otherwise if we have a key (we are doing "PCK modulation" rather than "noteid
         * modulation") find a voice and update that. Otherwise it is a monophonic modulation
         * so update every voice.
         */
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
        else if (pevt->key >= 0 && pevt->channel >= 0 && pevt->port_index >= 0)
        {
            // poly by PCK
            for (auto &v : voices)
            {
                if (v.key == pevt->key && v.channel == pevt->channel &&
                    v.portid == pevt->port_index)
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
    /*
     * Note expression handling is similar to polymod. Traverse the voices - in note expression
     * indexed by channel / key / port - and adjust the modulation slot in each.
     */
    case CLAP_EVENT_NOTE_EXPRESSION:
    {
        auto pevt = reinterpret_cast<const clap_event_note_expression *>(evt);
        for (auto &v : voices)
        {
            if (!v.isPlaying())
                continue;

            // Note expressions work on key not note id
            if (v.key == pevt->key && v.channel == pevt->channel && v.portid == pevt->port_index)
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

void ClapSawDemo::handleEventsFromUIQueue(const clap_output_events_t *ov)
{
    bool uiAdjustedValues{false};
    ClapSawDemo::FromUI r;
    while (fromUiQ.try_dequeue(r))
    {
        switch (r.type)
        {
        case FromUI::BEGIN_EDIT:
        case FromUI::END_EDIT:
        {
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

    // Similarly we need to push values to a UI on startup
    if (refreshUIValues && editor)
    {
        _DBGCOUT << "Pushing a refresh of UI values to the editor" << std::endl;
        refreshUIValues = false;

        for (const auto &[k, v] : paramToValue)
        {
            auto r = ToUI();
            r.type = ToUI::PARAM_VALUE;
            r.id = k;
            r.value = *v;
            toUiQ.try_enqueue(r);
        }
    }

    if (uiAdjustedValues)
        pushParamsToVoices();
}

/*
 * The note on, note off, and push params to voices implementations are, basically, completely
 * uninteresting.
 */
void ClapSawDemo::handleNoteOn(int port_index, int channel, int key, int noteid)
{
    bool foundVoice{false};
    for (auto &v : voices)
    {
        if (v.state == SawDemoVoice::OFF)
        {
            activateVoice(v, port_index, channel, key, noteid);
            foundVoice = true;
            break;
        }
    }

    if (!foundVoice)
    {
        // We could steal oldest. If you want to do that toss in a PR to add age
        // to the voice I guess. This is just a demo synth though.
        auto idx = rand() % max_voices;
        auto &v = voices[idx];
        terminatedVoices.emplace_back(v.portid, v.channel, v.key, v.note_id);
        activateVoice(v, port_index, channel, key, noteid);
    }

    dataCopyForUI.updateCount++;
    dataCopyForUI.polyphony++;

    if (editor)
    {
        auto r = ToUI();
        r.type = ToUI::MIDI_NOTE_ON;
        r.id = (uint32_t)key;
        toUiQ.try_enqueue(r);
    }
}

void ClapSawDemo::handleNoteOff(int port_index, int channel, int n)
{
    for (auto &v : voices)
    {
        if (v.isPlaying() && v.key == n && v.portid == port_index && v.channel == channel)
        {
            v.release();
        }
    }

    if (editor)
    {
        auto r = ToUI();
        r.type = ToUI::MIDI_NOTE_OFF;
        r.id = (uint32_t)n;
        toUiQ.try_enqueue(r);
    }
}

void ClapSawDemo::activateVoice(SawDemoVoice &v, int port_index, int channel, int key, int noteid)
{
    v.unison = std::max(1, std::min(7, (int)unisonCount));
    v.filterMode = (int)static_cast<int>(filterMode);
    v.note_id = noteid;
    v.portid = port_index;
    v.channel = channel;

    v.uniSpread = unisonSpread;
    v.oscDetune = oscDetune;
    v.cutoff = cutoff;
    v.res = resonance;
    v.preFilterVCA = preFilterVCA;
    v.ampRelease = scaleTimeParamToSeconds(ampRelease);
    v.ampAttack = scaleTimeParamToSeconds(ampAttack);
    v.ampGate = ampIsGate > 0.5;

    // reset all the modulations
    v.cutoffMod = 0;
    v.oscDetuneMod = 0;
    v.resMod = 0;
    v.preFilterVCAMod = 0;
    v.uniSpreadMod = 0;
    v.volumeNoteExpressionValue = 0;
    v.pitchNoteExpressionValue = 0;

    v.start(key);
}

/*
 * If the processing loop isn't running, the call to requestParamFlush from the UI will
 * result in this being called on the main thread, and generating all the appropriate
 * param updates.
 */
void ClapSawDemo::paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept
{
    auto sz = in->size(in);

    // This pointer is the sentinel to our next event which we advance once an event is processed
    for (auto e = 0U; e < sz; ++e)
    {
        auto nextEvent = in->get(in, e);
        handleInboundEvent(nextEvent);
    }

    handleEventsFromUIQueue(out);

    // We will never generate a note end event with processing active, and we have no midi
    // output, so we are done.
}

void ClapSawDemo::pushParamsToVoices()
{
    for (auto &v : voices)
    {
        if (v.isPlaying())
        {
            v.uniSpread = unisonSpread;
            v.oscDetune = oscDetune;
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
    auto cloc = std::locale("C");
    oss.imbue(cloc);
    oss << "STREAM-VERSION-1;";
    for (const auto &[id, val] : paramToValue)
    {
        oss << id << "=" << std::setw(30) << std::setprecision(20) << *val << ";";
    }
    _DBGCOUT << oss.str() << std::endl;

    auto st = oss.str();
    auto c = st.c_str();
    auto s = st.length() + 1; // write the null terminator
    while (s > 0)
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
    static constexpr uint32_t maxSize = 4096 * 8, chunkSize = 256;
    char buffer[maxSize];
    char *bp = &(buffer[0]);
    int64_t rd{0};
    int64_t totalRd{0};

    while ((rd = stream->read(stream, bp, chunkSize)) > 0)
    {
        bp += rd;
        totalRd += rd;
        if (totalRd >= maxSize - chunkSize - 1)
        {
            _DBGCOUT << "Invalid stream: Why did you send me so many bytes!" << std::endl;
            // What the heck? You sdent me more than 32kb of data for a 700 byte string?
            // That means my next chunk read will blow out memory so....
            return false;
        }
    }

    auto dat = std::string(buffer);
    _DBGCOUT << dat << std::endl;

    std::vector<std::string> items;
    size_t spos{0};
    while ((spos = dat.find(';')) != std::string::npos)
    {
        auto l = dat.substr(0, spos);
        dat = dat.substr(spos + 1);
        items.push_back(l);
    }

    if (items[0] != "STREAM-VERSION-1")
    {
        _DBGCOUT << "Invalid stream" << std::endl;
        return false;
    }
    for (auto i : items)
    {
        auto epos = i.find('=');
        if (epos == std::string::npos)
            continue; // oh well
        auto id = std::atoi(i.substr(0, epos).c_str());
        double val = 0.0;
        std::istringstream istr(i.substr(epos + 1));
        istr.imbue(std::locale("C"));
        istr >> val;

        *(paramToValue[(paramIds)id]) = val;
    }

    pushParamsToVoices();
    return true;
}

/*
 * A simple passthrough. Put it here to allow the template mechanics to see the impl.
 */
void ClapSawDemo::editorParamsFlush() { _host.paramsRequestFlush(); }

#if IS_LINUX
bool ClapSawDemo::registerTimer(uint32_t interv, clap_id *id)
{
    auto res = _host.timerSupportRegister(interv, id);
    return res;
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
