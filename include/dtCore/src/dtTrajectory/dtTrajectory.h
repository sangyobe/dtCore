// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTTRAJECTORY_H__
#define __DTCORE_DTTRAJECTORY_H__

/** \defgroup dtCore
 *
 * This module provides the trajectory interface for various trajectory
 * interpolation implementations.
 *
 * \code
 * #include <dtCore/dtTrajectory>
 * \endcode
 */

#include "dtMath/dtMath.h"

namespace dtCore {

enum class dtTrajType {
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

/**
 * class dtTrajectory
 * Interface definition for all trajectory implementations.
 */
template <typename m_type, dtTrajType m_trajType = dtTrajType::NONE>
class dtTrajectory {
public:
  // constructors
  dtTrajectory();
  dtTrajectory(const double t0, const double tf, const m_type &initial,
               const m_type &final);

  // destructor
  virtual ~dtTrajectory();

  // reconfigure trajecotry parameters
  void reconfigure(const double t0, const double tf, const m_type &initial,
                   const m_type &final);

  // get interpolated trajectory
  virtual void interpolate(const double t, m_type &current) const = 0;

protected:
  virtual void _determineCoeff(const m_type &initial, const m_type &final) = 0;

protected:
  double _t0;
  double _tf;
#ifdef _DEBUG
  m_type _p0;
  m_type _pf;
#endif
};
} // namespace dtCore

#include "dtTrajectory.tpp"

#endif // __DTCORE_DTTRAJECTORY_H__
