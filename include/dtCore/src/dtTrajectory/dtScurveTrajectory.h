// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSCURVETRAJECTORY_H__
#define __DTCORE_DTSCURVETRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtScurveTrajectory provides trajectory interpolation for one joint
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
 * dtScurveTrajectory<double, 3> traj(dtPolyType::CUBIC, t0, tf, pi, pf, vi,
 * vf, ai, af);
 *
 * double tc = 3.0;
 * dtVector<double, 3> p, v, a;
 * traj.interpolate(tc, p, v, a);
 *
 * \endcode
 */

#include "dtScurve.h"
#include <cstring> // memcpy

namespace dtCore {

/**
 * dtScurveTrajectory
 */
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
class dtScurveTrajectory 
{
public:
    typedef ValueType ValType;
    typedef ValueType *ContRefType;

public:
    dtScurveTrajectory();
    dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                      const ContRefType pi, const ContRefType pf,
                      const ValueType timeOffset = 0);
    dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                      const ContRefType pi, const ContRefType pf, 
                      const ContRefType vi, const ContRefType vf, 
                      const ValueType timeOffset = 0);
    dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                      const ContRefType pi, const ContRefType pf,
                      const ContRefType vi, const ContRefType vf,
                      const ContRefType ai, const ContRefType af,
                      const ValueType timeOffset = 0);
    dtScurveTrajectory(const ContRefType vl, const ContRefType al,
                      const ContRefType pi, const ContRefType pf,
                      const ValueType timeOffset = 0);
    dtScurveTrajectory(const ContRefType vl, const ContRefType al,
                      const ContRefType pi, const ContRefType pf,
                      const ContRefType vi, const ContRefType vf,
                      const ValueType timeOffset = 0);
    dtScurveTrajectory(const ContRefType vl, const ContRefType al,
                      const ContRefType pi, const ContRefType pf,
                      const ContRefType vi, const ContRefType vf,
                      const ContRefType ai, const ContRefType af,
                      const ValueType timeOffset = 0);
    ~dtScurveTrajectory();

public:
    virtual void Interpolate(const ValueType t, ContRefType p) const;
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v) const;
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const;

    virtual void ReConfigure();

    void SetParam(const ValueType duration, const ValueType accDuration, 
                  const ContRefType pi, const ContRefType pf,
                  const ValueType timeOffset = 0);
    void SetParam(const ValueType duration, const ValueType accDuration, 
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ValueType timeOffset = 0);
    void SetParam(const ValueType duration, const ValueType accDuration, 
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ContRefType ai, const ContRefType af,
                  const ValueType timeOffset = 0);
    void SetParam(const ContRefType vl, const ContRefType al,
                  const ContRefType pi, const ContRefType pf,
                  const ValueType timeOffset = 0);
    void SetParam(const ContRefType vl, const ContRefType al,
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ValueType timeOffset = 0);
    void SetParam(const ContRefType vl, const ContRefType al,
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ContRefType ai, const ContRefType af,
                  const ValueType timeOffset = 0);

    void SetDuration(const ValueType duration, const ValueType accDuration);
    void SetInitParam(const ContRefType pi, const ContRefType vi = nullptr, const ContRefType ai = nullptr);
    void SetTargetParam(const ContRefType pi, const ContRefType vf = nullptr, const ContRefType af = nullptr);
    void SetTimeOffset(const ValueType timeOffset);
    void SetLimit(const ContRefType vl, const ContRefType al);

private:
    void CalculateMaxVelocity();
    void CalculateDuration();

    ValueType m_ti;
    ValueType m_duration;
    ValueType m_accDuration;
    ValueType m_decDuration;
    ValueType m_conDuration;
    ValueType m_pi[m_dof];
    ValueType m_pf[m_dof];
    ValueType m_vi[m_dof];
    ValueType m_vf[m_dof];
    ValueType m_ai[m_dof];
    ValueType m_af[m_dof];
    ValueType m_vm[m_dof];
    ValueType m_vl[m_dof];
    ValueType m_al[m_dof];
    dtScurve<ValueType, m_order> m_interpolator[m_dof];
    bool m_isLimitSet = false;
};

} // namespace dtCore

#include "dtScurveTrajectory.tpp"

#endif // __DTCORE_DTSCURVETRAJECTORY_H__
