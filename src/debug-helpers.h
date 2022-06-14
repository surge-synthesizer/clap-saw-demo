/*
 * ClapSawDemo
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

#ifndef CLAP_SAW_DEMO_DEBUG_HELPERS_H
#define CLAP_SAW_DEMO_DEBUG_HELPERS_H

// These are just some macros I put in to trace certain lifecycle and value moments to stdout
#include <iostream>
#define _DBGCOUT                                                                                   \
    std::cout << "[clap-saw-demo] " << __FILE__ << ":" << __LINE__ << " (" << __func__ << ") : "
#define _DBGMARK                                                                                   \
    std::cout << "[clap-saw-demo] " << __FILE__ << ":" << __LINE__ << " (" << __func__ << ")"      \
              << std::endl;
#define _D(x) " [" << #x << "=" << x << "] "

#endif // CLAP_SAW_DEMO_DEBUG_HELPERS_H
