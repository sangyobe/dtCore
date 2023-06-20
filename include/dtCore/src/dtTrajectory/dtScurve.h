// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSCURVE_H__
#define __DTCORE_DTSCURVE_H__

#include <limits> // 

namespace dtCore {
/**
 * dtScurve : N-th s-curve interpolator
 */
template <typename ValueType, uint16_t m_order = 1> 
class dtScurve
{
public:
  dtScurve();
  virtual ~dtScurve();

public:
  virtual void Interpolate(const ValueType t, ValueType &p) const;
  virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v) const;
  virtual void Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const;

  virtual void Configure(const ValueType p0, const ValueType pf, 
                         const ValueType v0, const ValueType vf, 
                         const ValueType a0, const ValueType af,
                         const ValueType vm, const ValueType duration, 
                         const ValueType accDuration, const ValueType decDuration);

private:
    void Coefficient(const ValueType p0, const ValueType pf,
                     const ValueType v0, const ValueType vf,
                     const ValueType a0, const ValueType af,
                     const ValueType t, ValueType *coeff);

    ValueType m_tolerance = std::numeric_limits<ValueType>::epsilon();
    ValueType m_duration;
    ValueType m_accDuration;
    ValueType m_decDuration;
    ValueType m_accCoeff[m_order + 1];
    ValueType m_conCoeff[m_order + 1];
    ValueType m_decCoeff[m_order + 1];
};

} // namespace dtCore

#include "dtScurve.tpp"

#endif // __DTCORE_DTSCURVE_H__