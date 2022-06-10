/*
 * StupiSaw is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

#ifndef STUPISAW_H
#define STUPISAW_H
#include <iostream>
#define _DBGCOUT std::cout << __FILE__ << ":" << __LINE__ << " (" << __func__ << ") :"
#define _DBGMARK std::cout << __FILE__ << ":" << __LINE__ << " (" << __func__ << ")" << std::endl;

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

#include "stupivoice.h"
#include <memory>

namespace BaconPaul
{
#if USE_MAC_UI
struct StupiSawMacUI;
#endif

struct StupiEditor;

struct StupiSaw : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                                               clap::helpers::CheckingLevel::Maximal>
{
    StupiSaw(const clap_host *host);
    ~StupiSaw();

    StupiEditor *editor{nullptr};

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

        pmCutoff = 17,
        pmResonance = 94,
    };
    static constexpr int nParams = 6;

    /*
     * Each paramId maps to an atomic double. I use a std::unordered_map for
     * that but of course other ways could work (like switch statements at the
     * appropriate point or so on)
     */
    std::atomic<double> unisonCount{3}, unisonSpread{10}, cutoff{69}, resonance{0.7},
        ampAttack{0.01}, ampRelease{0.2}, filterDecay{0.2}, filterModDepth{0};
    std::unordered_map<clap_id, std::atomic<double> *> paramToValue;

    // "Voice Management" is "have 64 and if you run out oh well"
    std::array<StupiVoice, 64> voices;

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

    /*
     * Parameter implementation is obvious and straight forward
     */
    bool implementsParams() const noexcept override;
    bool isValidParamId(clap_id paramId) const noexcept override;
    uint32_t paramsCount() const noexcept override;
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override;

    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream *strea) noexcept override { return true; }
    bool stateLoad(const clap_istream *strea) noexcept override { return true; }

  public:
    // Finally I have a static description
    static clap_plugin_descriptor desc;

    bool implementsGui() const noexcept override { return true; }
    bool guiIsApiSupported(const char *api, bool isFloating) noexcept override { return true; }

  protected:
    bool guiCreate(const char *api, bool isFloating) noexcept override;
    void guiDestroy() noexcept override;

  public:
    bool guiSetParent(const clap_window *window) noexcept override;

    static constexpr uint32_t guiw = 500, guih = 300;
    bool guiGetSize(uint32_t *width, uint32_t *height) noexcept override
    {
        *width = guiw;
        *height = guih;
        return true;
    }

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

    typedef moodycamel::ReaderWriterQueue<ToUI, 4096> SynthToUI_Queue_t;
    typedef moodycamel::ReaderWriterQueue<FromUI, 4096> UIToSynth_Queue_t;

    SynthToUI_Queue_t toUiQ;
    UIToSynth_Queue_t fromUiQ;
};
} // namespace BaconPaul

#endif