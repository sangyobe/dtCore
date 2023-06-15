// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTPOLYNOMIALTRAJECTORY_H__
#define __DTCORE_DTPOLYNOMIALTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtPolynomialTrajectory provides trajectory interpolation for one joint
 * using polynomial interpolator. Degree of polynomial can be specified
 * as a template variable.
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

#include "dtPolynomial.h"
#include <cstring> // memcpy

namespace dtCore {

/**
 * dtPolynomialTrajectory
 */
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
class dtPolynomialTrajectory 
{
public:
  typedef ValueType ValType;
  typedef ValueType *ContRefType;

public:
  dtPolynomialTrajectory();
  dtPolynomialTrajectory(const ValueType duration,
                         const ContRefType pi, const ContRefType pf, 
                         const ValueType timeOffset = 0);
  dtPolynomialTrajectory(const ValueType duration, 
                         const ContRefType pi, const ContRefType pf, 
                         const ContRefType vi, const ContRefType vf, 
                         const ValueType timeOffset = 0);
  dtPolynomialTrajectory(const ValueType duration, 
                         const ContRefType pi, const ContRefType pf, 
                         const ContRefType vi, const ContRefType vf, 
                         const ContRefType ai, const ContRefType af, 
                         const ValueType timeOffset = 0);
  ~dtPolynomialTrajectory();

public:
  virtual void Interpolate(const ValueType t, ContRefType p) const;
  virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v) const;
  virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const;

  virtual void ReConfigure();

  void SetParam(const ValueType duration, 
                const ContRefType pi, const ContRefType pf,
                const ValueType timeOffset = 0);
  void SetParam(const ValueType duration, 
                const ContRefType pi, const ContRefType pf,
                const ContRefType vi, const ContRefType vf,
                const ValueType timeOffset = 0);
  void SetParam(const ValueType duration, 
                const ContRefType pi, const ContRefType pf,
                const ContRefType vi, const ContRefType vf,
                const ContRefType ai, const ContRefType af,
                const ValueType timeOffset = 0);
  void SetDuration(const ValueType duration);
  void SetPoint(const ContRefType pi, const ContRefType pf,
                const ContRefType vi = nullptr, const ContRefType vf = nullptr,
                const ContRefType ai = nullptr, const ContRefType af = nullptr);
  void SetTimeOffset(const ValueType timeOffset);

private:
  ValueType m_ti;
  ValueType m_duration;
  ValueType m_pi[m_dof];
  ValueType m_pf[m_dof];
  ValueType m_vi[m_dof];
  ValueType m_vf[m_dof];
  ValueType m_ai[m_dof];
  ValueType m_af[m_dof];

  dtPolynomial<ValueType, m_order> m_interpolator[m_dof];
};

} // namespace dtCore

#include "dtPolynomialTrajectory.tpp"

#endif // __DTCORE_DTPOLYNOMIALTRAJECTORY_H__
