// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTBEZIER_H__
#define __DTCORE_DTBEZIER_H__

#include <cstring> // memcpy
#include <cmath>

namespace dtCore {

/*! \brief dtBezier: 1 dof, N'th bezier trajectory
    \details
    This class provides 1 degree of freedom and n'th bezier trajectory.
    \param[in] ValueType float or double
    \param[in] m_maxNum n'th bezier
*/
template <typename ValueType, uint16_t m_maxNum> 
class dtBezier 
{
public:
    dtBezier();
    virtual ~dtBezier();

public:
    void Interpolate(const ValueType t, ValueType &p) const; //!< Calculates the desired position(p) corresponding to the time(t) entered. 
    void Interpolate(const ValueType t, ValueType &p, ValueType &v) const; //!< Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
    void Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const; //!< Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 

    void Configure(const ValueType p0, const ValueType pf,
                   const ValueType v0, const ValueType vf,
                   const ValueType a0, const ValueType af,
                   const ValueType *pc, const uint8_t pcSize, const ValueType duration); //!< Configure the control points and coefficients of the bezier trajectory from the parameters entered.

private:
    ValueType BinomialCoeff(const int n, const int k) const; //!< Calculate binomial coefficients.

    uint16_t m_num; //!< Bezier control point num (pcSize + Init parameter (3) + Target paramter (3))
    ValueType m_duration; //!< Bezier trajectory duration
    ValueType m_p[m_maxNum + 6]; //!< Bezier control point
    ValueType m_posCoeff[m_maxNum + 6]; //!< Binomial coefficient for position
    ValueType m_velCoeff[m_maxNum + 5]; //!< Binomial coefficient for velocity
    ValueType m_accCoeff[m_maxNum + 4]; //!< Binomial coefficient for acceleration
};

} // namespace dtCore

#include "dtBezier.tpp"

#endif // __DTCORE_DTBEZIER_H__