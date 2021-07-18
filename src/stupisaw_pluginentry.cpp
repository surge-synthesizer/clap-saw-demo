#include "stupisaw.h"

#include <iostream>
#include <cmath>
#include <cstring>


namespace BaconPaul
{

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
