// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTBEZIERTRAJECTORY_H__
#define __DTCORE_DTBEZIERTRAJECTORY_H__

/**
 *
 * dtBezierTrajectory provides ....
 *
 * \code
 * #include <dtCore/dtTrajectory>
 *
 * double t0 = 0.0;
 * double tf = 10.0;
 * dtVector<double, 3> pi, pf, vi, vf, ai, af;
 * pi << 0.0, 5.0, -10.0;
 * pf << 5.0, -5.0, 0.0;
 * vi.Zero();
 * vf.Zero();
 * ai.Zero();
 * af.Zero();
 * dtPolynomialTrajectory<double, 3> traj(dtPolyType::CUBIC, t0, tf, pi, pf, vi,
 * vf, ai, af);
 *
 * double tc = 3.0;
 * dtVector<double, 3> p, v, a;
 * traj.interpolate(tc, p, v, a);
 *
 * \endcode
 */

#include "dtBezier.h"

namespace dtCore {

/**
 * dtBezierTrajectory
 */
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
class dtBezierTrajectory {
public:
  typedef ValueType ValType;
  typedef ValueType *ContType;
  typedef ValueType *ContRefType;

public:
  dtBezierTrajectory(const ValueType duration, const ContRefType pi,
                     const ContRefType pf, const ContRefType pc,
                     const ValueType timeOffset = 0);
  ~dtBezierTrajectory();

public:
  virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v,
                           ContRefType a) const;

  virtual void Reconfigure();

  void SetTimeOffset(const ValueType timeOffset);
  void SetDuration(const ValueType duration);
  void SetInitialParam(const ContRefType pi);
  void SetTargetParam(const ContRefType pf);
  void SetControlParam(const ContRefType pc);

private:
  ValueType m_duration;
  ValueType m_ti;
  ValueType m_tf;
  ValueType m_pi[m_dof];
  ValueType m_pf[m_dof];
  dtBezier<ValueType, m_order> m_interpolator[m_dof];
  ValueType m_pc[m_dof][m_order - 1];
};

} // namespace dtCore

#include "dtBezierTrajectory.tpp"

#endif // __DTCORE_DTBEZIERTRAJECTORY_H__
