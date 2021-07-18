/*
 * StupiSaw is Free and Open Source released under the MIT license
 *
 * Copright (c) 2021, Paul Walker
 */

/*
 * This file provides the `clap_plugin_entry` entry point required in the DLL for all
 * clap plugins. It is almost definitely the case that in this simple single plugin instance
 * this code could be provided as a template by the core clap library or plugin glue.
 *
 * The key insight here is that the actual plugin class (BaconPaul::StupiSaw) contains a
 * static clap_plugin_description member which I return here at the key points. Almost
 * all the rest of this is boilerplate.
 */

#include "stupisaw.h"

#include <iostream>
#include <cmath>
#include <cstring>

namespace BaconPaul
{

namespace Adapters
{
bool clap_init(const char *p) { return true; }
void clap_deinit(void) {}
uint32_t clap_get_plugin_count() { return 1; }
const clap_plugin_descriptor *clap_get_plugin_descriptor(uint32_t w) { return &StupiSaw::desc; }

static const clap_plugin *clap_create_plugin(const clap_host *host, const char *plugin_id)
{
    if (strcmp(plugin_id, StupiSaw::desc.id))
    {
        std::cout << "Warning: CLAP asked for plugin_id '" << plugin_id << "' and stupisaw ID is '"
                  << StupiSaw::desc.id << "'" << std::endl;
        return nullptr;
    }
    auto p = new StupiSaw(host);
    return p->clapPlugin();
}

static uint32_t clap_get_invalidation_sources_count(void) { return 0; }

static const clap_plugin_invalidation_source *clap_get_invalidation_sources(uint32_t index)
{
    return nullptr;
}

static void clap_refresh(void) {}
} // namespace Adapters
} // namespace BaconPaul

extern "C"
{
    const CLAP_EXPORT struct clap_plugin_entry clap_plugin_entry = {
        CLAP_VERSION,
        BaconPaul::Adapters::clap_init,
        BaconPaul::Adapters::clap_deinit,
        BaconPaul::Adapters::clap_get_plugin_count,
        BaconPaul::Adapters::clap_get_plugin_descriptor,
        BaconPaul::Adapters::clap_create_plugin,
        BaconPaul::Adapters::clap_get_invalidation_sources_count,
        BaconPaul::Adapters::clap_get_invalidation_sources,
        BaconPaul::Adapters::clap_refresh,
    };
}
