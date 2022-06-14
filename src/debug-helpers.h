//
// Created by Paul Walker on 6/13/22.
//

#ifndef CLAP_SAW_DEMO_DEBUG_HELPERS_H
#define CLAP_SAW_DEMO_DEBUG_HELPERS_H

// TODO: Condition this on release build and so on of course
#include <iostream>
#define _DBGCOUT std::cout << "[clap-saw-demo] " << __FILE__ << ":" << __LINE__ << " (" << __func__ << ") : "
#define _DBGMARK std::cout << "[clap-saw-demo] " << __FILE__ << ":" << __LINE__ << " (" << __func__ << ")" << std::endl;
#define _D(x) " [" << #x << "=" << x << "] "

#endif // CLAP_SAW_DEMO_DEBUG_HELPERS_H
