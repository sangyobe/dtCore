// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTHTRANSFORMTRAJECTORY_H__
#define __DTCORE_DTHTRANSFORMTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtHTransformTrajectory provides 3D pose trajectory interpolation in task space.
 *
 * \code
 * #include <dtCore/dtTrajectory>
 * 
 * double ti = 0.0;
 * double tf = 10.0;
 * dtHTransform<double> Ti, Tf;
 * Ti << 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0;
 * Tf << 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0;
 * dtHTransformTrajectory<double> traj(t0, tf, Ti, Tf);
 * 
 * double tc = 3.0;
 * dtHTransform<double> Tc;
 * traj.interpolate(tc, Tc);
 * 
 * \endcode
 */

#include "dtMath/dtMath.h"

namespace dtCore {

template <typename ValueType> class dtHTransformTrajectory {
public:
  typedef ValueType ValType;
  typedef Eigen::Matrix<ValueType, 4, 4> ContType;
  typedef Eigen::Matrix<ValueType, 4, 4> &ContRefType;

public:
  dtHTransformTrajectory();
  dtHTransformTrajectory(const ValueType t0, const ValueType tf,
                         const ContRefType initial, const ContRefType final);
  virtual ~dtHTransformTrajectory();

public:
  virtual void Interpolate(const ValueType t, ContRefType current) const;

protected:
  virtual void Reconfigure(const ContRefType initial, const ContRefType final);
};

} // namespace dtCore

#include "dtHTransformTrajectory.tpp"

#endif // __DTCORE_DTHTRANSFORMTRAJECTORY_H__