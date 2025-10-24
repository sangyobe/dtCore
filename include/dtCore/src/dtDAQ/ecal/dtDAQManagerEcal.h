// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ__DAQMANAGERECAL_H__
#define __DT_DAQ__DAQMANAGERECAL_H__

/** ]addtogroup dtDAQ
 *
 */

#include "../dtDAQManager.h"
#include <ecal/ecal.h>

namespace dt
{
namespace DAQ
{

class DAQManagerEcal : public DAQManager
{
public:
    DAQManagerEcal() {}
    virtual ~DAQManagerEcal() {}

    void Initialize() {
        eCAL::Initialize();
        eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "sub info");
    }
    void Terminate() {
        
    }
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ__DAQMANAGERECAL_H__