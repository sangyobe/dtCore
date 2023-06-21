// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSCURVE_H__
#define __DTCORE_DTSCURVE_H__

#include <limits> // 

namespace dtCore {

/*! \brief dtScurve: 1 dof, n'th s-curve trajectory
    \details
    This class provides 1 degree of freedom and n'th s-curve trajectory.
    \param[in] ValueType float or double
    \param[in] m_order n'th polynomial
*/
template <typename ValueType, uint16_t m_order = 1> 
class dtScurve
{
public:
  dtScurve();
  virtual ~dtScurve();

public:
  virtual void Interpolate(const ValueType t, ValueType &p) const; //!< Calculates the desired position(p) corresponding to the time(t) entered. 
  virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v) const; //!< Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
  virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const; //!< Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 

  virtual void Configure(const ValueType p0, const ValueType pf, 
                         const ValueType v0, const ValueType vf, 
                         const ValueType a0, const ValueType af,
                         const ValueType vm, const ValueType duration, 
                         const ValueType accDuration, const ValueType decDuration); //!< Configure the coefficients of the s-curve from the parameters entered.

private:
    void Coefficient(const ValueType p0, const ValueType pf,
                     const ValueType v0, const ValueType vf,
                     const ValueType a0, const ValueType af,
                     const ValueType t, ValueType *coeff); // Calculate s-curve coefficient.

    ValueType m_tolerance = std::numeric_limits<ValueType>::epsilon(); //!< Threshold to prevent being divided by zero
    ValueType m_duration; //!< s-curve trajectory duration
    ValueType m_accDuration; //!< s-curve trajectory acceleration section duration
    ValueType m_decDuration; //!< s-curve trajectory deceleration section duration
    ValueType m_accCoeff[m_order + 1]; //!< the coefficients of the s-curve acceleration section
    ValueType m_conCoeff[m_order + 1]; //!< the coefficients of the s-curve const velocity section
    ValueType m_decCoeff[m_order + 1]; //!< the coefficients of the s-curve deceleration section
};

} // namespace dtCore

#include "dtScurve.tpp"

#endif // __DTCORE_DTSCURVE_H__