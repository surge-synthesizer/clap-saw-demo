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
namespace Stupisaw
{
bool clap_init(const char *p) { return true; }
void clap_deinit() {}
uint32_t clap_get_plugin_count(const clap_plugin_factory *f) { return 1; }
const clap_plugin_descriptor *clap_get_plugin_descriptor(const clap_plugin_factory *f, uint32_t w)
{
    return &StupiSaw::desc;
}

static const clap_plugin *clap_create_plugin(const clap_plugin_factory *f, const clap_host *host,
                                             const char *plugin_id)
{
    _DBGCOUT << "Creating StupiSaw" << std::endl;
    if (strcmp(plugin_id, StupiSaw::desc.id))
    {
        std::cout << "Warning: CLAP asked for plugin_id '" << plugin_id << "' and stupisaw ID is '"
                  << StupiSaw::desc.id << "'" << std::endl;
        return nullptr;
    }
    auto p = new StupiSaw(host);
    return p->clapPlugin();
}

static const CLAP_EXPORT struct clap_plugin_factory stupisaw_factory = {
    BaconPaul::Stupisaw::clap_get_plugin_count,
    BaconPaul::Stupisaw::clap_get_plugin_descriptor,
    BaconPaul::Stupisaw::clap_create_plugin,
};
static const void *get_factory(const char *factory_id) { return &stupisaw_factory; }
} // namespace Stupisaw
} // namespace BaconPaul

extern "C"
{
    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {
        CLAP_VERSION, BaconPaul::Stupisaw::clap_init, BaconPaul::Stupisaw::clap_deinit,
        BaconPaul::Stupisaw::get_factory};
}
