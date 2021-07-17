#include "stupisaw.h"

#include <iostream>


namespace BaconPaul
{
StupiSaw::StupiSaw(const clap_plugin_descriptor *desc, const clap_host *host)
: clap::Plugin(desc, host)
{
    std::cout << "Creating a stupisaw" << std::endl;
}
}

extern "C"
{
    const CLAP_EXPORT struct clap_plugin_entry clap_plugin_entry = {
        CLAP_VERSION,
        nullptr, // clap_init,
        nullptr, // clap_deinit,
        nullptr, // clap_get_plugin_count,
        nullptr, // clap_get_plugin_descriptor,
        nullptr, // clap_create_plugin,
        nullptr, // clap_get_invalidation_sources_count,
        nullptr, // clap_get_invalidation_sources,
        nullptr, // clap_refresh,
    };
}