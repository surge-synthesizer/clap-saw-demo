#include "stupisaw.h"

#include <iostream>


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
                paramId == pmResonance);
    return res;
}
bool StupiSaw::paramsInfo(int32_t paramIndex, clap_param_info *info) const noexcept
{
    if (paramIndex >= nParams) return false;
    info->module[0] = 0;
    switch( paramIndex )
    {
    case 0:
        info->id = pmUnisonCount;
        strncpy(info->name, "Unison Count", CLAP_NAME_SIZE);
        info->min_value = 1;
        info->max_value = 7;
        info->default_value = 3;
        break;
    case 1:
        info->id = pmUnisonSpread;
        strncpy(info->name, "Unison Spread in Cents", CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 100;
        info->default_value = 10;
        break;
    case 2:
        info->id = pmCutoff;
        strncpy(info->name, "Cutoff in Keys", CLAP_NAME_SIZE);
        info->min_value = 1;
        info->max_value = 127;
        info->default_value = 69;
        break;
    case 3:
        info->id = pmResonance;
        strncpy(info->name, "Resonance", CLAP_NAME_SIZE);
        info->min_value = 0.0;
        info->max_value = 1.0;
        info->default_value = 0.7;
        break;
    }
    return true;
}
bool StupiSaw::paramsValue(clap_id paramId, double *value) noexcept
{
    std::cout << "paramsValue " << paramId << std::endl;
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
    }

    return true;
}

namespace Adapters
{
bool clap_init(const char* p)
{
    std::cout << "clap_init " << p << std::endl;
    return true;
}
void clap_deinit(void)
{
    std::cout << "clap_deinit" << std::endl;
}
uint32_t clap_get_plugin_count()
{
    std::cout << "clap_get_plugin_count" << std::endl;
    return 1;
}
const clap_plugin_descriptor *clap_get_plugin_descriptor(uint32_t w)
{
    std::cout << "clap_get_plugin_descriptor " << w << std::endl;
    return &StupiSaw::desc;
}

static const clap_plugin *clap_create_plugin(const clap_host *host, const char* plugin_id)
{
    std::cout << "clap_create_plugin " << host << " " << plugin_id << std::endl;
    if (strcmp(plugin_id, "org.baconpaul.stupisaw"))
    {
        std::cout << "Odd - asked for the wrong thing" << std::endl;
    }
    auto p = new StupiSaw(host);
    return p->clapPlugin();
}

static uint32_t clap_get_invalidation_sources_count(void)
{
    return 0;
}

static const clap_plugin_invalidation_source *clap_get_invalidation_sources(uint32_t index)
{
    return nullptr;
}

static void clap_refresh(void)
{
}
}
}

extern "C"
{
    const CLAP_EXPORT struct clap_plugin_entry clap_plugin_entry = {
        CLAP_VERSION,
        BaconPaul::Adapters::clap_init,
        BaconPaul::Adapters::clap_deinit,
        BaconPaul::Adapters::clap_get_plugin_count,
        BaconPaul::Adapters::clap_get_plugin_descriptor, // clap_get_plugin_descriptor,
        BaconPaul::Adapters::clap_create_plugin, // clap_create_plugin,
        BaconPaul::Adapters::clap_get_invalidation_sources_count, // clap_get_invalidation_sources_count,
        BaconPaul::Adapters::clap_get_invalidation_sources, // clap_get_invalidation_sources,
        BaconPaul::Adapters::clap_refresh, // clap_refresh,
    };
}