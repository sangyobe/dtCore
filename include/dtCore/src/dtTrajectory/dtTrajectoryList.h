// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTTRAJECTORYLIST_H__
#define __DTCORE_DTTRAJECTORYLIST_H__

/** \defgroup dtCore
 *
 * This module provides a list of trajectories.
 * A list of trajectories consist of a starting point, multiple via points,
 * and a final point.
 *
 * \code
 * #include <dtCore/dtTrajectoryList.h>
 * \endcode
 */

#include "dtTrajectory.h"
#include <list>

namespace dtCore {

template <typename m_type, dtTrajType m_trajType> class dtTrajectoryList {
public:
  dtTrajectoryList() {}
  virtual ~dtTrajectoryList() {}

  void Add(const dtTrajectory<m_type, m_trajType> &traj);

private:
  std::list<dtTrajectory<m_type, m_trajType>> trajList_;
};

} // namespace dtCore

#endif // __DTCORE_DTTRAJECTORYLIST_H__