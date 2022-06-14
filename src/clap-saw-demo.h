/*
 * ClapSawDemo is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#ifndef CLAP_SAW_DEMO_H
#define CLAP_SAW_DEMO_H
#include <iostream>
#include "debug-helpers.h"

/*
 * This header file is the clap::Plugin from the plugin-glue helpers which gives
 * me an object implementing the clap protocol. It contains parameter maps, voice
 * management - all the stuff you would expect.
 *
 * The synth itself is a unison saw wave polysynth with a terrible ipmlementation
 * of a saw wave generator, a simple biquad LPF, and a few dumb linear envelopes.
 * It is not a synth you would want to use for anything other than demoware
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

    ClapSawDemoEditor *editor{nullptr};

    /*
     * I have a set of parameter Ids which are purposefully non-contiguous to
     * make sure I use the APIs correctly
     */
    enum paramIds
    {
        pmUnisonCount = 1378,
        pmUnisonSpread = 2391,
        pmAmpAttack = 2874,
        pmAmpRelease = 728,
        pmAmpIsGate = 1942,

        pmPreFilterVCA = 87612,

        pmCutoff = 17,
        pmResonance = 94,
        pmFilterMode = 14255
    };
    static constexpr int nParams = 9;

    // These items are ONLY read and written on the audio thread, so they
    // are safe to be non-atomic doubles. We keep a map to locate them
    // for parameter updates.
    double unisonCount{3}, unisonSpread{10}, cutoff{69}, resonance{0.7},
        ampAttack{0.01}, ampRelease{0.2}, ampIsGate{0}, preFilterVCA{1.0}, filterMode{0};
    std::unordered_map<clap_id, double *> paramToValue;

    // "Voice Management" is "have 64 and if you run out oh well"
    std::array<SawDemoVoice, 64> voices;

  protected:
    /*
     * Plugin Setup. Activate makes sure sampleRate is distributed through
     * the data structures, and the audio ports setup sets up a single
     * stereo output.
     */
    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override;

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override;
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool isInput) const noexcept override { return isInput ? 1 : 0; }
    bool notePortsInfo(uint32_t index, bool isInput,
                       clap_note_port_info *info) const noexcept override;

    /*
     * Process manages the events (midi and automation) and generates the
     * sound of course by summing across active voices
     */
    clap_process_status process(const clap_process *process) noexcept override;
    void handleInboundEvent(const clap_event_header_t *evt);
    void pushParamsToVoices();
    void handleNoteOn(int key, int noteid);
    void handleNoteOff(int key);
    float scaleTimeParamToSeconds(float param); // 0->1 linear input to 0 -> 5 exp output

    /*
     * Parameter implementation is obvious and straight forward
     */
    bool implementsParams() const noexcept override;
    bool isValidParamId(clap_id paramId) const noexcept override;
    uint32_t paramsCount() const noexcept override;
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override;
    bool paramsValueToText(clap_id paramId, double value, char *display,
                           uint32_t size) noexcept override;

    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream *strea) noexcept override;
    bool stateLoad(const clap_istream *strea) noexcept override;

    bool implementsVoiceInfo() const noexcept override { return true; }
    bool voiceInfoGet(clap_voice_info *info) noexcept override
    {
        info->voice_capacity = 64;
        info->voice_count = 64;
        info->flags = CLAP_VOICE_INFO_SUPPORTS_OVERLAPPING_NOTES;
        return true;
    }

  public:
    // Finally I have a static description
    static clap_plugin_descriptor desc;

  protected:
    // These are implemented in clap-saw-demo-editor.
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
    bool implementsTimerSupport() const noexcept override {
        _DBGMARK;
        return true;
    }
    void onTimer(clap_id timerId) noexcept override ;

    bool registerTimer(uint32_t interv, clap_id *id);
    bool unregisterTimer(clap_id id);

    bool implementsPosixFdSupport() const noexcept override {
        _DBGMARK;
        return true;
    }
    void onPosixFd(int fd, int flags) noexcept override;
    bool registerPosixFd(int fd);
    bool unregisterPosixFD(int fd);

#endif

  public:
    static constexpr uint32_t GUI_DEFAULT_W = 330, GUI_DEFAULT_H = 530;

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
};
} // namespace sst::clap_saw_demo

#endif