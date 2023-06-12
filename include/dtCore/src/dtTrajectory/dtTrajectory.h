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

namespace dtCore {

template <typename ValueType, uint16_t m_dof = 1, uint16_t m_degree = 1>
class dtTrajectory {
public:
  typedef ValueType *ContType;
  typedef ValueType *ContRefType;

public:
  dtTrajectory(const ValueType duration, const ContRefType pi,
               const ContRefType pf, const ContRefType vi = 0,
               const ContRefType vf = 0, const ContRefType ai = 0,
               const ContRefType af = 0, const ValueType timeOffset = 0);
  virtual ~dtTrajectory();

public:
  void Interpolate(const ValueType t, ContRefType p, ContRefType v,
                   ContRefType a) const;

  void SetTimeOffset(const ValueType timeOffset);
  void SetDuration(const ValueType duration);
  void SetInitialParam(const ContRefType pi, const ContRefType vi,
                       const ContRefType ai);
  void SetTargetParam(const ContRefType pf, const ContRefType vf,
                      const ContRefType af);

protected:
  class dtPolynomial {
  public:
    dtPolynomial();
    virtual ~dtPolynomial();

  public:
    virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v,
                             ValueType &a) const;

    virtual void DetermineCoeff(const ValueType duration, const ValueType &p0,
                                const ValueType &pf, const ValueType &v0,
                                const ValueType &vf, const ValueType &a0,
                                const ValueType &af);

  private:
    ValueType m_coeff[m_degree + 1];
    ValueType m_duration;
    ValueType m_p0;
    ValueType m_pf;
  };

  void ReconfigurePolynomial();

private:
  dtPolynomial m_poly[m_dof];
  ValueType m_duration;
  ValueType m_ti;
  ValueType m_tf;
  ValueType m_pi[m_dof];
  ValueType m_pf[m_dof];
  ValueType m_vi[m_dof];
  ValueType m_vf[m_dof];
  ValueType m_ai[m_dof];
  ValueType m_af[m_dof];
};

} // namespace dtCore

#include "dtTrajectory.tpp"

#endif // __DTCORE_DTTRAJECTORY_H__
