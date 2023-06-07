// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTORIENTATIONTRAJECTORY_H__
#define __DTCORE_DTORIENTATIONTRAJECTORY_H__

/** \defgroup dtCore
 *
 * This module provides a list of trajectories.
 * A list of trajectories consist of a starting point, multiple via points,
 * and a final point.
 *
 * \code
 * #include <dtCore/dtOrientationTrajectory.h>
 * \endcode
 */

#include "dtTrajectory.h"

namespace dtCore {

template <typename m_type, dtTrajType m_trajType>
class dtOrientationTrajectory : public dtTrajectory<m_type, m_trajType> {
public:
  dtOrientationTrajectory() {}
  dtOrientationTrajectory(const double t0, const double tf,
                          const m_type &initial, const m_type &final) {}
  virtual ~dtOrientationTrajectory() {}

public:
  virtual void interpolate(const double t, m_type &current) const override {}

protected:
  virtual void _determineCoeff(const m_type &initial,
                               const m_type &final) override {}
};

} // namespace dtCore

#endif // __DTCORE_DTORIENTATIONTRAJECTORY_H__