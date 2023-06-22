// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTBEZIERTRAJECTORY_H__
#define __DTCORE_DTBEZIERTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtBezierTrajectory provides n'th bezier trajectory with one or more degrees of freedom.
 * The degree of freedom can be specified as a template variable. ////
 * The order(n) of polynomial can be specified as a template variable. ////
 *
 * p(t) (v(t), a(t))
 *  |                                   pc[0]
 *  |                                     o                                         pf (vf, af) 
 *  |                                                         pc[pcNum - 1]           o
 *  |                                                               o                 |
 *  |                                            pc[1]    ...                         |
 *  |                       pi (vi, ai)            o                                  |
 *  |                         o                                                       |
 *  |                         |                                                       |
 *  |                         |                                                       |
 *  +-------------------------------------------------------------------------------------------------- t
 *  t0 <--- time offset --->  ti        <---          duration         --->          tf
 *
 */

#include "dtBezier.h"

namespace dtCore {

/*! \brief dtBezierTrajectory: Trajectories with a degree of freedom of m_dof and up to m_maxNum control points
    \details
    This class provides trajectories with a degree of freedom of m_dof and up to m_maxNum control points
    \param[in] ValueType float or double
    \param[in] m_dof m degree of freedom
    \param[in] m_maxNum max input control point num
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_maxNum>
class dtBezierTrajectory 
{
public:
    typedef ValueType ValType;
    typedef ValueType *ContType;
    typedef ValueType *ContRefType;

public:
    dtBezierTrajectory(); //!< Initialize without input parameter
    dtBezierTrajectory(const ValueType duration, 
                      const ContRefType pi, const ContRefType pf, 
                      const ContRefType pc, const uint16_t pcNum,
                      const ValueType timeOffset = 0); //!< Configure control points and coefficients of the bezier from the parameters entered.
    dtBezierTrajectory(const ValueType duration, 
                      const ContRefType pi, const ContRefType pf, 
                      const ContRefType vi, const ContRefType vf, 
                      const ContRefType pc, const uint16_t pcNum,
                      const ValueType timeOffset = 0); //!< Configure control points and coefficients of the bezier from the parameters entered.
    dtBezierTrajectory(const ValueType duration, 
                      const ContRefType pi, const ContRefType pf, 
                      const ContRefType vi, const ContRefType vf, 
                      const ContRefType ai, const ContRefType af, 
                      const ContRefType pc, const uint16_t pcNum,
                      const ValueType timeOffset = 0); //!< Configure control points and coefficients of the bezier from the parameters entered.
    dtBezierTrajectory(const ValueType duration, 
                      const ContRefType pi, const ContRefType pf, 
                      const ContRefType vi, const ContRefType vf, 
                      const ContRefType ai, const ContRefType af, 
                      const ContRefType *pc[m_dof], const uint16_t pcNum,
                      const ValueType timeOffset = 0); //!< Configure control points and coefficients of the bezier from the parameters entered.
    ~dtBezierTrajectory();

public:
    virtual void Interpolate(const ValueType t, ContRefType p) const; //!< Calculates the desired position(p) corresponding to the time(t) entered. 
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v) const; //!< Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
    virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const; //!< Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 

    virtual void Reconfigure(); //!< Reconfigure the coefficients of polynomial through parameters entered from functions below (SetParam, SetDuration, SetInitParam, SetTargetParam, SetControlParam, SetTimeOffset).

    void SetParam(const ValueType duration, 
                  const ContRefType pi, const ContRefType pf, 
                  const ContRefType pc, const uint16_t pcNum,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure function.
    void SetParam(const ValueType duration, 
                  const ContRefType pi, const ContRefType pf, 
                  const ContRefType vi, const ContRefType vf, 
                  const ContRefType pc, const uint16_t pcNum,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure function.
    void SetParam(const ValueType duration, 
                  const ContRefType pi, const ContRefType pf, 
                  const ContRefType vi, const ContRefType vf, 
                  const ContRefType ai, const ContRefType af, 
                  const ContRefType pc, const uint16_t pcNum,
                  const ValueType timeOffset = 0); //!< Enter parameters for the Configure function.

    void SetDuration(const ValueType duration);  //!< Enter trajectory duration for the Configure function.
    void SetInitParam(const ContRefType pi, const ContRefType vi = nullptr, const ContRefType ai = nullptr); //!< Enter init parameter for the Configure function.
    void SetTargetParam(const ContRefType pf, const ContRefType vf = nullptr, const ContRefType af = nullptr); //!< Enter target parameter for the Configure function.
    void SetControlParam(const ContRefType pc, const uint16_t pcNum); //!< Enter input contorl parameter for the Configure function.
    void SetTimeOffset(const ValueType timeOffset); //!< Enter trajectory delay for the Configure function.

private:
    ValueType m_ti; //!< trajectory time offset(delay) (sec)
    ValueType m_duration; //!< trajectory duration (sec)
    ValueType m_pi[m_dof]; //!< init position (x)
    ValueType m_pf[m_dof]; //!< target position (x)
    ValueType m_vi[m_dof]; //!< init velocity (x/sec)
    ValueType m_vf[m_dof]; //!< target velocity (x/sec)
    ValueType m_ai[m_dof]; //!< init acceleration (x/sec^2)
    ValueType m_af[m_dof]; //!< target accleration (x/sec^2)
    ValueType m_pc[m_dof][m_maxNum + 6]; //!< input control point (x)
    uint16_t m_pcNum; //!< number of input control point

    dtBezier<ValueType, m_maxNum> m_interpolator[m_dof]; //!< dtBezier trajectory
};

} // namespace dtCore

#include "dtBezierTrajectory.tpp"

#endif // __DTCORE_DTBEZIERTRAJECTORY_H__
