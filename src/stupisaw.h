#include <clap-plugin.hh>
#include <atomic>

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
        pmCutoff = 17,
        pmResonance = 94
    };
    static constexpr int nParams = 4;

    std::atomic<double> unisonCount{3}, unisonSpread{10}, cutoff{69}, resonance{0.7};

  protected:
    bool implementsParams() const noexcept override;
    bool isValidParamId(clap_id paramId) const noexcept override;
    uint32_t paramsCount() const noexcept override;
    bool paramsInfo(int32_t paramIndex, clap_param_info *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override;

  public:
    static clap_plugin_descriptor desc;
};
}
