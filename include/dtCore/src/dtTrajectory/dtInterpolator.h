// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTINTERPOLATOR_H__
#define __DTCORE_DTINTERPOLATOR_H__

/**
 *
 */

namespace dtCore {

// enum class dtInterpolatorType : std::uint16_t {
//   NONE = 0,
//   LINEAR = 1,
//   QUADRATIC = 2,
//   CUBIC = 3,
//   QUINTIC = 4,
//   JERK = 5,
//   // CSPLINE_NATURAL = 11,
//   // CSPLINE_CLAMPED,
//   // CSPLINE_FORCED,
//   // LINEAR_PARABOLIC_BLEND = 21, // S-Curve
//   // BEZIER_2D,
//   // BEZIER_3D
// };

// enum class dtPolynomialType : std::uint16_t {
//   NONE = dtInterpolatorType::NONE,
//   LINEAR = dtInterpolatorType::LINEAR,
//   QUADRATIC = dtInterpolatorType::QUADRATIC,
//   CUBIC = dtInterpolatorType::CUBIC,
//   QUINTIC = dtInterpolatorType::QUINTIC,
//   JERK = dtInterpolatorType::JERK,
// };

template <typename ValueType> class dtInterpolator {
public:
  dtInterpolator() {}
  virtual ~dtInterpolator() {}
  virtual void Interpolate(ValueType t, ValueType &p, ValueType &v,
                           ValueType &a) const = 0;
};

template <typename ValueType, uint16_t m_degree>
class dtPolynomialInterpolator : public dtInterpolator<ValueType> {
public:
public:
  dtPolynomialInterpolator(ValueType duration, ValueType p0, ValueType pf,
                           ValueType v0 = 0, ValueType vf = 0, ValueType a0 = 0,
                           ValueType af = 0);
  virtual ~dtPolynomialInterpolator();
  virtual void Interpolate(ValueType t, ValueType &p, ValueType &v,
                           ValueType &a) const override;

private:
  void _determinCoeff();

private:
  ValueType m_coeff[m_degree + 1];
  ValueType m_duration;
#ifdef _DEBUG
  ValueType m_p0;
  ValueType m_pf;
  ValueType m_v0;
  ValueType m_vf;
  ValueType m_a0;
  ValueType m_af;
#endif
};

template <typename ValueType, uint16_t m_degree = 2>
class dtBezierInterpolator : public dtInterpolator<ValueType> {
public:
  dtBezierInterpolator(ValueType duration, ValueType p0, ValueType pf,
                       ValueType *tc, ValueType *pc);
  virtual ~dtBezierInterpolator();
  virtual void Interpolate(ValueType t, ValueType &p, ValueType &v,
                           ValueType &a) const override;

private:
  ValueType m_duration;
  ValueType m_p0;
  ValueType m_pf;
  ValueType m_tc[m_degree + 1];
  ValueType m_pc[m_degree + 1];
};

template <typename ValueType>
class dtLinearParabolicBlendInterpolator : public dtInterpolator<ValueType> {
public:
  dtLinearParabolicBlendInterpolator(ValueType p0, ValueType pf,
                                     ValueType linearVel, ValueType acc);
  virtual ~dtLinearParabolicBlendInterpolator();
  virtual void Interpolate(ValueType t, ValueType &p, ValueType &v,
                           ValueType &a) const override;

private:
  ValueType m_duration;
  ValueType m_p0;
  ValueType m_pf;
  ValueType m_acc;
  ValueType m_vel;
};

} // namespace dtCore

#include "dtInterpolator.tpp"

#endif // __DTCORE_DTINTERPOLATOR_H__