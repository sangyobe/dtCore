// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTBEZIER_H__
#define __DTCORE_DTBEZIER_H__

#include <cstring> // memcpy

namespace dtCore {

/**
 * dtBezier : N-th Bezier interpolator
 */
template <typename ValueType, uint16_t m_degree> class dtBezier {
public:
  dtBezier();
  virtual ~dtBezier();

public:
  void Interpolate(const ValueType t, ValueType &p, ValueType &v,
                   ValueType &a) const;

  void Configure(const ValueType p0, const ValueType pf, const ValueType *pc,
                 ValueType duration = 1.0);

private:
  ValueType m_duration;
  ValueType
      m_p[m_degree + 1]; // contains initial, final and control points.
                         // m_p[0]: initial value, m_p[m_degree]: final value
};

} // namespace dtCore

#include "dtBezier.tpp"

#endif // __DTCORE_DTBEZIER_H__