// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTTRAJECTORY_H__
#define __DTCORE_DTTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtTrajectory is the trajectory interface for various trajectory
 * interpolator implementations. It provides a unit trajectory interpolation.
 *
 */

#include "dtMath/dtMath.h"

namespace dtCore {

enum class dtPolyType {
  NONE = 0,
  LINEAR = 1,
  QUADRATIC = 2,
  CUBIC = 3,
  QUINTIC = 4,
  JERK = 5,
  // LINEAR_PARABOLIC_BLEND = 6
  // CSPLINE_NATURAL,
  // CSPLINE_CLAMPED,
  // CSPLINE_FORCED
};

} // namespace dtCore

#endif // __DTCORE_DTTRAJECTORY_H__
