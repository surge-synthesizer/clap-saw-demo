/*
 * ClapSawDemo
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

#ifndef CLAP_SAW_DEMO_H
#define CLAP_SAW_DEMO_H
#include <iostream>
#include "debug-helpers.h"

/*
 * ClapSawDemo is the core synthesizer class. It uses the clap-helpers C++ plugin extensions
 * to present the CLAP C API as a C++ object model.
 *
 * The core features here are
 *
 * - Hold the CLAP description static object
 * - Advertise parameters and ports
 * - Provide an event handler which responds to events and returns sound
 * - Do voice management. Which is really not very sophisticated (it's just an array of 64
 *   voice objects and we choose the next free one, and if you ask for a 65th voice, nothing
 *   happens).
 * - Provide the API points to delegate UI creation to a separate editor object,
 *   coded in clap-saw-demo-editor
 *
 * This demo is coded to be relatively familiar and close to programming styles form other
 * formats where the editor and synth collaborate closely; as described in clap-saw-demo-editor
 * this object also holds the two queues the editor and synth use to communicate; and holds the
 * bundle of atomic values to which the editor holds a const &.
 */

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>
#include <readerwriterqueue.h>

#include "saw-voice.h"
#include <memory>

namespace sst::clap_saw_demo
{

struct ClapSawDemoEditor;

struct ClapSawDemo : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                                                  clap::helpers::CheckingLevel::Maximal>
{
    ClapSawDemo(const clap_host *host);
    ~ClapSawDemo();

    /*
     * This static (defined in the cpp file) allows us to present a name, feature set,
     * url etc... and is consumed by clap-saw-demo-pluginentry.cpp
     */
    static clap_plugin_descriptor desc;

    /*
     * Activate makes sure sampleRate is distributed through
     * the data structures, in this case by stamping the sampleRate
     * onto each pre-allocated voice object.
     */
    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        for (auto &v : voices)
            v.sampleRate = sampleRate;
        return true;
    }

    /*
     * Parameter Handling:
     *
     * Each parameter gets a unique ID which is returned by 'paramsInfo' when the plugin
     * is instantiated (or the plugin asks the host to reset). To avoid accidental bugs where
     * I confuse creation index with param IDs, I am using arbitrary numbers for each
     * parameter id.
     *
     * The implementation of paramsInfo contains the setup of these params.
     *
     * The actual synth has a very simple model to update parameter values. It
     * contains a map from these IDs to a double * which the constructor sets up
     * as references to members.
     */
    enum paramIds : uint32_t
    {
        pmUnisonCount = 1378,
        pmUnisonSpread = 2391,
        pmOscDetune = 8675309,

        pmAmpAttack = 2874,
        pmAmpRelease = 728,
        pmAmpIsGate = 1942,

        pmPreFilterVCA = 87612,

        pmCutoff = 17,
        pmResonance = 94,
        pmFilterMode = 14255
    };
    static constexpr int nParams = 10;

    bool implementsParams() const noexcept override { return true; }
    bool isValidParamId(clap_id paramId) const noexcept override
    {
        return paramToValue.find(paramId) != paramToValue.end();
    }
    uint32_t paramsCount() const noexcept override { return nParams; }
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override
    {
        *value = *paramToValue[paramId];
        return true;
    }
    bool paramsValueToText(clap_id paramId, double value, char *display,
                           uint32_t size) noexcept override;
    // Convert 0-1 linear into 0-4s exponential
    float scaleTimeParamToSeconds(float param);

    /*
     * Many CLAP plugins will want input and output audio and note ports, altough
     * the spec doesn't require this. Here as a simple synth we set up a single s
     * stereo output and a single midi / clap_note input.
     */
    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override { return isInput ? 0 : 1; }
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool isInput) const noexcept override { return isInput ? 1 : 0; }
    bool notePortsInfo(uint32_t index, bool isInput,
                       clap_note_port_info *info) const noexcept override;

    /*
     * VoiceInfo is an optional (currently draft) extension where you advertise
     * polyphony information. Crucially here though it allows you to advertise that
     * you can support overlapping notes, which in conjunction with CLAP_DIALECT_NOTE
     * and the Bitwig voice stack modulator lets you stack this little puppy!
     */
    bool implementsVoiceInfo() const noexcept override { return true; }
    bool voiceInfoGet(clap_voice_info *info) noexcept override
    {
        info->voice_capacity = 64;
        info->voice_count = 64;
        info->flags = CLAP_VOICE_INFO_SUPPORTS_OVERLAPPING_NOTES;
        return true;
    }

    /*
     * I have an unacceptably crude state dump and restore. If you want to
     * improve it, PRs welcome! But it's just like any other read-and-write-goop
     * from-a-stream api really.
     */
    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream *) noexcept override;
    bool stateLoad(const clap_istream *) noexcept override;

    /*
     * process is the meat of the operation. It does obvious things like trigger
     * voices but also handles all the polypohnic modulation and so on. Please see the
     * comments in the cpp file to understand it and the helper functions we have
     * delegated to.
     */
    clap_process_status process(const clap_process *process) noexcept override;
    void handleInboundEvent(const clap_event_header_t *evt);
    void pushParamsToVoices();
    void handleNoteOn(int port_index, int channel, int key, int noteid);
    void handleNoteOff(int port_index, int channel, int key);

  protected:
    /*
     * GUI IMPLEMENTATION with VSTGUI and thread safe queues
     *
     * Most importantly, the gui code is all implemented in clap-saw-demo-editor.cpp,
     * including the functions here which are members of ClapSawDemo.
     */
    bool implementsGui() const noexcept override { return true; }
    bool guiIsApiSupported(const char *api, bool isFloating) noexcept override;

    bool guiCreate(const char *api, bool isFloating) noexcept override;
    void guiDestroy() noexcept override;
    bool guiSetParent(const clap_window *window) noexcept override;

    bool guiCanResize() const noexcept override { return true; }
    bool guiAdjustSize(uint32_t *width, uint32_t *height) noexcept override;
    bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
    bool guiGetSize(uint32_t *width, uint32_t *height) noexcept override
    {
        *width = GUI_DEFAULT_W;
        *height = GUI_DEFAULT_H;
        return true;
    }

#if IS_LINUX
  public:
    bool implementsTimerSupport() const noexcept override
    {
        _DBGMARK;
        return true;
    }
    void onTimer(clap_id timerId) noexcept override;

    bool registerTimer(uint32_t interv, clap_id *id);
    bool unregisterTimer(clap_id id);

    bool implementsPosixFdSupport() const noexcept override
    {
        _DBGMARK;
        return true;
    }
    void onPosixFd(int fd, int flags) noexcept override;
    bool registerPosixFd(int fd);
    bool unregisterPosixFD(int fd);

#endif

  public:
    static constexpr uint32_t GUI_DEFAULT_W = 390, GUI_DEFAULT_H = 530;

  public:
    /*
     * We use three basic data structures to communicate between the UI and the
     * DSP code. Two queues of data for parameter updates and notes one structure
     * full of atomics which we read in our idle loop for repaints with an atomic
     * update count.
     */
    struct ToUI
    {
        enum MType
        {
            PARAM_VALUE = 0x31,
            MIDI_NOTE_ON,
            MIDI_NOTE_OFF
        } type;

        uint32_t id;  // param-id for PARAM_VALUE, key for noteon/noteoff
        double value; // value or unused
    };

    struct FromUI
    {
        enum MType
        {
            BEGIN_EDIT = 0xF9,
            END_EDIT,
            ADJUST_VALUE
        } type;
        uint32_t id;
        double value;
    };

    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<int> polyphony{0};
    } dataCopyForUI;

    typedef moodycamel::ReaderWriterQueue<ToUI, 4096> SynthToUI_Queue_t;
    typedef moodycamel::ReaderWriterQueue<FromUI, 4096> UIToSynth_Queue_t;

    SynthToUI_Queue_t toUiQ;
    UIToSynth_Queue_t fromUiQ;

  private:
    ClapSawDemoEditor *editor{nullptr};

    // These items are ONLY read and written on the audio thread, so they
    // are safe to be non-atomic doubles. We keep a map to locate them
    // for parameter updates.
    double unisonCount{3}, unisonSpread{10}, oscDetune{0}, cutoff{69}, resonance{0.7},
        ampAttack{0.01}, ampRelease{0.2}, ampIsGate{0}, preFilterVCA{1.0}, filterMode{0};
    std::unordered_map<clap_id, double *> paramToValue;

    // "Voice Management" is "have 64 and if you run out oh well"
    std::array<SawDemoVoice, 64> voices;
};
} // namespace sst::clap_saw_demo

#endif