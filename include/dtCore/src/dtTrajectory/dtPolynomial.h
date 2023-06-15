// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTPOLYNOMIAL_H__
#define __DTCORE_DTPOLYNOMIAL_H__

namespace dtCore {

/**
 * dtPolynomial : N-th polynomial interpolator
 */
template <typename ValueType, uint16_t m_order = 1> 
class dtPolynomial 
{
public:
  dtPolynomial();
  virtual ~dtPolynomial();

public:
  virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v,
                           ValueType &a) const;

  virtual void Configure(const ValueType p0, const ValueType pf,
                         const ValueType v0, const ValueType vf,
                         const ValueType a0, const ValueType af,
                         const ValueType duration);

private:
  ValueType m_coeff[m_order + 1];
  ValueType m_duration;
  ValueType m_p0;
  ValueType m_pf;
};

} // namespace dtCore

#include "dtPolynomial.tpp"

#endif // __DTCORE_DTPOLYNOMIAL_H__