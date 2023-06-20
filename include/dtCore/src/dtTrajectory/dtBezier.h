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

/**
 * dtBezier : N-th Bezier interpolator
 */
template <typename ValueType, uint16_t m_order> 
class dtBezier 
{
public:
    dtBezier();
    virtual ~dtBezier();

public:
    void Interpolate(const ValueType t, ValueType &p) const;
    void Interpolate(const ValueType t, ValueType &p, ValueType &v) const;
    void Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const;

    void Configure(const ValueType p0, const ValueType pf,
                   const ValueType v0, const ValueType vf,
                   const ValueType a0, const ValueType af,
                   const ValueType *pc, const uint8_t pcSize, const ValueType duration);
                                             
                                             
private:
    ValueType BinomialCoeff(const int n, const int k) const;
    ValueType BernsteinPoly(int n, int k, ValueType ctrlParm) const;

    int m_num;
    ValueType m_duration;
    ValueType m_p[m_order + 5];
    ValueType m_coeff[m_order + 5];
};

} // namespace dtCore

#include "dtBezier.tpp"

#endif // __DTCORE_DTBEZIER_H__