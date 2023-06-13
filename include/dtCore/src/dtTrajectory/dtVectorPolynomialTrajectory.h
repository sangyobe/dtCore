// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTVECTORPOLYNOMIALTRAJECTORY_H__
#define __DTCORE_DTVECTORPOLYNOMIALTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtVectorPolynomialTrajectory provides trajectory interpolation for one joint
 * using polynomial interpolator. Degree of polynomial can be specified
 * as a template variable.
 *
 * \code
 * #include <dtCore/dtTrajectory>
 *
 * double t0 = 0.0;
 * double tf = 10.0;
 * dtVector<double, 3> pi, pf, vi, vf, ai, af;
 * pi << 0.0, 5.0, -10.0;
 * pf << 5.0, -5.0, 0.0;
 * vi.Zero();
 * vf.Zero();
 * ai.Zero();
 * af.Zero();
 * dtVectorPolynomialTrajectory<double, 3> traj(dtPolyType::CUBIC, t0, tf, pi,
 * pf, vi, vf, ai, af);
 *
 * double tc = 3.0;
 * dtVector<double, 3> p, v, a;
 * traj.interpolate(tc, p, v, a);
 *
 * \endcode
 */

#include <dtMath/dtMath.h>

namespace dtCore {

enum class dtPolyType {
  NONE = 0,
  LINEAR = 1,
  QUADRATIC = 2,
  CUBIC = 3,
  QUINTIC = 4,
  JERK = 5,
  // LINEAR_PARABOLIC_BLEND = 6
  // CSPLINE_NATURAL,
  // CSPLINE_CLAMPED,
  // CSPLINE_FORCED
};

template <typename ValueType, uint32_t DOF> class dtVectorPolynomialTrajectory {
public:
  typedef ValueType ValType;
  typedef Eigen::Matrix<ValueType, DOF, 1> ContType;
  typedef Eigen::Matrix<ValueType, DOF, 1> &ContRefType;

public:
  dtVectorPolynomialTrajectory();
  dtVectorPolynomialTrajectory(dtPolyType trajType, const ValueType t0,
                               const ValueType tf, const ContRefType p0,
                               const ContRefType pf);
  dtVectorPolynomialTrajectory(dtPolyType trajType, const ValueType t0,
                               const ValueType tf, const ContRefType p0,
                               const ContRefType pf, const ContRefType v0,
                               const ContRefType vf);
  dtVectorPolynomialTrajectory(dtPolyType trajType, const ValueType t0,
                               const ValueType tf, const ContRefType p0,
                               const ContRefType pf, const ContRefType v0,
                               const ContRefType vf, const ContRefType a0,
                               const ContRefType af);
  virtual ~dtVectorPolynomialTrajectory();

public:
  virtual void Interpolate(const ValueType t, ContRefType p, ContRefType v,
                           ContRefType a) const;

protected:
  virtual void Reconfigure();

private:
  dtPolyType _trajType;
  ValueType _t0;
  ValueType _tf;
  ContType _p0;
  ContType _pf;
  ContType _v0;
  ContType _vf;
  ContType _a0;
  ContType _af;
  dtMath::VectorXd _coeff;
};

} // namespace dtCore

#include "dtVectorPolynomialTrajectory.tpp"

#endif // __DTCORE_DTVECTORPOLYNOMIALTRAJECTORY_H__