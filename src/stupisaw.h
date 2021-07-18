#include <clap-plugin.hh>
#include <atomic>
#include <array>

#include "stupivoice.h"

#ifndef STUPISAW_H
#define STUPISAW_H

namespace BaconPaul
{
struct StupiSaw : public clap::Plugin
{
    StupiSaw(const clap_host *host);

    /*
     * It's a saw wave with unison and an LPF. That's it. Force param IDs to be
     * wierdo numbers to avoid accidentally using them with ++ and stuff
     */
    enum paramIds
    {
        pmUnisonCount = 1378,
        pmUnisonSpread = 2391,
        pmAmpAttack = 2874,
        pmAmpRelease = 728,

        pmCutoff = 17,
        pmResonance = 94,

        pmFilterDecay = 24032,
        pmFilterModDepth = 238
    };
    static constexpr int nParams = 8;

    std::atomic<double> unisonCount{3}, unisonSpread{10}, cutoff{69}, resonance{0.7},
        ampAttack{0.01}, ampRelease{0.2}, filterDecay{0.2}, filterModDepth{0};


    std::array<StupiVoice, 64> voices;

  protected:
    bool activate(double sampleRate) noexcept override;
    clap_process_status process(const clap_process *process) noexcept override;
    uint32_t audioPortsCount(bool isInput) const noexcept override;
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

  protected:
    bool implementsParams() const noexcept override;
    bool isValidParamId(clap_id paramId) const noexcept override;
    uint32_t paramsCount() const noexcept override;
    bool paramsInfo(int32_t paramIndex, clap_param_info *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override;
    bool startProcessing() noexcept override;

    // GUI
    bool implementsGuiCocoa() const noexcept override;
    bool guiCocoaAttach(void *nsView) noexcept override;

  public:
    static clap_plugin_descriptor desc;
};
}

#endif