// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTPOLYNOMIAL_H__
#define __DTCORE_DTPOLYNOMIAL_H__

namespace dtCore {

/*! \brief dtPolynomial: 1 dof, N'th polynomial trajectory
    \details
    This class generates 1 degree of freedom and n'th polynomial trajectory.
    \param[in] ValueType float or double
    \param[in] m_order n'th polynomial
*/
template <typename ValueType, uint16_t m_order = 1> 
class dtPolynomial 
{
public:
  dtPolynomial();
  virtual ~dtPolynomial();

public:
  virtual void Interpolate(const ValueType t, ValueType &p) const; //!< Calculates the desired position(p) corresponding to the time(t) entered. 
  virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v) const; //!< Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
  virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const; //!< Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 

  virtual void Configure(const ValueType p0, const ValueType pf,
                         const ValueType v0, const ValueType vf,
                         const ValueType a0, const ValueType af,
                         const ValueType duration); //!< Obtain the coefficients of the polynomial from the parameters entered.

private:
  ValueType m_coeff[m_order + 1]; //!< the coefficients of the polynomial
};

} // namespace dtCore

#include "dtPolynomial.tpp"

#endif // __DTCORE_DTPOLYNOMIAL_H__