// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDAQMANAGERECAL_H__
#define __DTCORE_DTDAQMANAGERECAL_H__

/** \defgroup dtDAQ
 *
 */

#include "../dtDAQManager.h"
#include <ecal/ecal.h>

namespace dtCore {

class dtDAQManagerEcal : public dtDAQManager {
public:
    dtDAQManagerEcal() {}
    virtual ~dtDAQManagerEcal() {}

    void Initialize() {
        eCAL::Initialize();
        eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "sub info");
    }
    void Terminate() {
        
    }
};

}

#endif // __DTCORE_DTDAQMANAGERECAL_H__