// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSCURVETRAJECTORY_H__
#define __DTCORE_DTSCURVETRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtPolynomialTrajectory provides n'th polynomial trajectory with one or more degrees of freedom.
 * The degree of freedom can be specified as a template variable.
 * The order(n) of polynomial can be specified as a template variable.
 *
 * v(t)
 *  |--vLimit--
 *  |                     ___________ vm____________
 *  |                    /                          \
 *  |                   /|                           \
 *  |                  / | aLimit                         \
 *  |                 /__|                             \
 *  |                / 1                                \
 *  |               /                                    \
 *  |          vi  /                                      \ vf
 *  +-------------o----------------------------------------o------- t
 *  t0 <-     ->  ti 
 *  | time offset |  acc  |<- const vel duration ->|  dec  |
 *                 duration                         duration
 */

#include "dtScurve.h"
#include <cstring> // memcpy

namespace dtCore {

/*! \brief dtScurveTrajectory: m dof, n-th order s-curve trajectory
    \details
    This class provides m degree of freedom and n'th s-curve trajectory.
    \param[in] ValueType float or double
    \param[in] m_dof m degree of freedom
    \param[in] m_order n-th order
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
class dtScurveTrajectory 
{
public:
    typedef ValueType ValType;
    typedef ValueType *ContRefType;

public:
    dtScurveTrajectory(); //!< Initialize without input parameter
    dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                      const ContRefType pi, const ContRefType pf,
                      const ValueType timeOffset = 0); //!< Initialize and configure the coefficients of the s-curve from the parameters entered.
    dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                      const ContRefType pi, const ContRefType pf, 
                      const ContRefType vi, const ContRefType vf, 
                      const ValueType timeOffset = 0); //!< Initialize and configure the coefficients of the s-curve from the parameters entered.
    dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                      const ContRefType pi, const ContRefType pf,
                      const ContRefType vi, const ContRefType vf,
                      const ContRefType ai, const ContRefType af,
                      const ValueType timeOffset = 0); //!< Initialize and configure the coefficients of the s-curve from the parameters entered.
    dtScurveTrajectory(const ContRefType vLimit, const ContRefType aLimit,
                      const ContRefType pi, const ContRefType pf,
                      const ValueType timeOffset = 0); //!< Initialize and configure the coefficients of the s-curve from the parameters entered.
    dtScurveTrajectory(const ContRefType vLimit, const ContRefType aLimit,
                      const ContRefType pi, const ContRefType pf,
                      const ContRefType vi, const ContRefType vf,
                      const ValueType timeOffset = 0); //!< Initialize and configure the coefficients of the s-curve from the parameters entered.
    dtScurveTrajectory(const ContRefType vLimit, const ContRefType aLimit,
                      const ContRefType pi, const ContRefType pf,
                      const ContRefType vi, const ContRefType vf,
                      const ContRefType ai, const ContRefType af,
                      const ValueType timeOffset = 0); //!< Initialize and configure the coefficients of the s-curve from the parameters entered.
    ~dtScurveTrajectory();

public:
    virtual void Interpolate(const ValueType t, ContRefType p) const; //!< Calculates the desired position(p) corresponding to the time(t) entered. 
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v) const; //!< Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const; //!< Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 

    virtual void Configure(); //!< Configure the coefficients of s-curve through parameters entered from functions below (SetParam, SetDuration, SetInitParam, SetTargetParam, SetTimeOffset, SetLimit).

    void SetParam(const ValueType duration, const ValueType accDuration, 
                  const ContRefType pi, const ContRefType pf,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure() function.
    void SetParam(const ValueType duration, const ValueType accDuration, 
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure() function.
    void SetParam(const ValueType duration, const ValueType accDuration, 
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ContRefType ai, const ContRefType af,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure() function.
    void SetParam(const ContRefType vLimit, const ContRefType aLimit,
                  const ContRefType pi, const ContRefType pf,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure() function.
    void SetParam(const ContRefType vLimit, const ContRefType aLimit,
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure() function.
    void SetParam(const ContRefType vLimit, const ContRefType aLimit,
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ContRefType ai, const ContRefType af,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure() function.

    void SetDuration(const ValueType duration, const ValueType accDuration); //!< Enter trajectory duration for the Configure() function.
    void SetInitParam(const ContRefType pi, const ContRefType vi = nullptr, const ContRefType ai = nullptr); //!< Enter init parameter for the Configure() function.
    void SetTargetParam(const ContRefType pi, const ContRefType vf = nullptr, const ContRefType af = nullptr); //!< Enter target parameter for the Configure() function.
    void SetTimeOffset(const ValueType timeOffset); //!< Enter trajectory offset(delay) for the Configure() function.
    void SetLimit(const ContRefType vLimit, const ContRefType aLimit); //!< Enter trajectory limit for the Configure() function.
    ValueType GetDuration() { return m_duration; } //!< Return trajectory duration.
    
private:
    void CalculateLinearVelocity(); //!< Calculate linear velocity.
    void CalculateDuration(); //!< Calculate the longest acceleration, constant velocity, and deceleration durations for the entire degree of freedom.

    ValueType m_tolerance = std::numeric_limits<ValueType>::epsilon(); //!< Threshold to prevent being divided by zero
    ValueType m_ti; //!< trajectory time offset(delay) (sec)
    ValueType m_duration; //!< trajectory duration (sec)
    ValueType m_accDuration; //!< trajectory acceleration duration (sec)
    ValueType m_decDuration; //!< trajectory const velocity duration (sec)
    ValueType m_conDuration; //!< trajectory deceleartion duration (sec)
    ValueType m_pi[m_dof]; //!< init position (x)
    ValueType m_pf[m_dof]; //!< target position (x)
    ValueType m_vi[m_dof]; //!< init velocity (x/sec)
    ValueType m_vf[m_dof]; //!< target velocity (x/sec)
    ValueType m_ai[m_dof]; //!< init acceleration (x/sec^2)
    ValueType m_af[m_dof]; //!< target accleration (x/sec^2)
    ValueType m_vLimit[m_dof]; //!< limit velocity (x/sec)
    ValueType m_aLimit[m_dof]; //!< limit accleration (x/sec^2)
    bool m_isLimitSet = false; //!< Is limit parameter entered?
    
    dtScurve<ValueType, m_order> m_interpolator[m_dof]; //!< dtScurve trajectory
};

} // namespace dtCore

#include "dtScurveTrajectory.tpp"

#endif // __DTCORE_DTSCURVETRAJECTORY_H__
