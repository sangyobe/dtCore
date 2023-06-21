// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTPOLYNOMIALTRAJECTORY_H__
#define __DTCORE_DTPOLYNOMIALTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtPolynomialTrajectory provides n'th polynomial trajectory with one or more degrees of freedom.
 * The degree of freedom can be specified as a template variable.
 * The order(n) of polynomial can be specified as a template variable.
 *
 * p(t) (v(t), a(t))
 *  |
 *  |                                                   pf (vf, af) 
 *  |                                            ...   ___o
 *  |                                        .            |
 *  |                                      /              |
 *  |                       pi (vi, ai)  .                |
 *  |                         o___ ...                    |
 *  |                         |                           |
 *  |                         |                           |
 *  +-------------------------------------------------------------- t
 *  t0 <--- time offset --->  ti   <--- duration --->    tf
 *
 */

#include "dtPolynomial.h"
#include <cstring> // memcpy

namespace dtCore {

/*! \brief dtPolynomialTrajectory: M dof, N'th polynomial trajectory
    \details
    This class provides m degree of freedom and n'th polynomial trajectory.
    \param[in] ValueType float or double
    \param[in] m_dof m degree of freedom
    \param[in] m_order n'th polynomial
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
class dtPolynomialTrajectory 
{
public:
    typedef ValueType ValType;
    typedef ValueType *ContRefType;

public:
    dtPolynomialTrajectory(); //!< Initialize without input parameter
    dtPolynomialTrajectory(const ValueType duration,
                          const ContRefType pi, const ContRefType pf, 
                          const ValueType timeOffset = 0); //!< Configure the coefficients of the polynomial from the parameters entered.
    dtPolynomialTrajectory(const ValueType duration, 
                          const ContRefType pi, const ContRefType pf, 
                          const ContRefType vi, const ContRefType vf, 
                          const ValueType timeOffset = 0); //!< Configure the coefficients of the polynomial from the parameters entered.
    dtPolynomialTrajectory(const ValueType duration, 
                          const ContRefType pi, const ContRefType pf, 
                          const ContRefType vi, const ContRefType vf, 
                          const ContRefType ai, const ContRefType af, 
                          const ValueType timeOffset = 0); //!< Configure the coefficients of the polynomial from the parameters entered.
    ~dtPolynomialTrajectory();

public:
    virtual void Interpolate(const ValueType t, ContRefType p) const; //!< Calculates the desired position(p) corresponding to the time(t) entered. 
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v) const; //!< Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const; //!< Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 

    virtual void ReConfigure(); //!< Reconfigure the coefficients of polynomial through parameters entered from functions below (SetParam, SetDuration, SetInitParam, SetTargetParam, SetTimeOffset).

    void SetParam(const ValueType duration, 
                  const ContRefType pi, const ContRefType pf,
                  const ValueType timeOffset = 0); //!< Enter parameters for the ReConfigure function.
    void SetParam(const ValueType duration, 
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ValueType timeOffset = 0); //!< Enter parameters for the ReConfigure function.
    void SetParam(const ValueType duration, 
                  const ContRefType pi, const ContRefType pf,
                  const ContRefType vi, const ContRefType vf,
                  const ContRefType ai, const ContRefType af,
                  const ValueType timeOffset = 0); //!< Enter parameters for the ReConfigure function.
    void SetDuration(const ValueType duration); //!< Enter trajectory duration for the ReConfigure function.
    void SetInitParam(const ContRefType pi, const ContRefType vi = nullptr, const ContRefType ai = nullptr); //!< Enter init parameter for the ReConfigure function.
    void SetTargetParam(const ContRefType pi, const ContRefType vf = nullptr, const ContRefType af = nullptr); //!< Enter target parameter for the ReConfigure function.
    void SetTimeOffset(const ValueType timeOffset); //!< Enter trajectory delay for the ReConfigure function.

private:
    ValueType m_ti; //!< trajectory time offset(delay) (sec)
    ValueType m_duration; //!< trajectory duration (sec)
    ValueType m_pi[m_dof]; //!< init position (x)
    ValueType m_pf[m_dof]; //!< target position (x)
    ValueType m_vi[m_dof]; //!< init velocity (x/sec)
    ValueType m_vf[m_dof]; //!< target velocity (x/sec)
    ValueType m_ai[m_dof]; //!< init acceleration (x/sec^2)
    ValueType m_af[m_dof]; //!< target accleration (x/sec^2)

    dtPolynomial<ValueType, m_order> m_interpolator[m_dof]; //!< dtPolynomial trajectory
};

} // namespace dtCore

#include "dtPolynomialTrajectory.tpp"

#endif // __DTCORE_DTPOLYNOMIALTRAJECTORY_H__
