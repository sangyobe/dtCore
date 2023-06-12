// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTPOLYNOMIALTRAJECTORY_H__
#define __DTCORE_DTPOLYNOMIALTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtPolynomialTrajectory provides trajectory interpolation for one joint
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
 * dtPolynomialTrajectory<double, 3> traj(dtPolyType::CUBIC, t0, tf, pi, pf, vi,
 * vf, ai, af);
 *
 * double tc = 3.0;
 * dtVector<double, 3> p, v, a;
 * traj.interpolate(tc, p, v, a);
 *
 * \endcode
 */

#include "dtInterpolator.h"

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

template <typename ValueType, uint32_t DOF> class dtPolynomialTrajectory {
public:
  typedef Eigen::Matrix<ValueType, DOF, 1> ContainerType;

public:
  dtPolynomialTrajectory();
  dtPolynomialTrajectory(dtPolyType trajType, const double t0, const double tf,
                         const ContainerType &p0, const ContainerType &pf,
                         const ContainerType &v0 = ContainerType::Zero(),
                         const ContainerType &vf = ContainerType::Zero(),
                         const ContainerType &a0 = ContainerType::Zero(),
                         const ContainerType &af = ContainerType::Zero());
  virtual ~dtPolynomialTrajectory();

public:
  virtual void interpolate(const double t, ContainerType &p, ContainerType &v,
                           ContainerType &a) const;

protected:
  virtual void _determineCoeff(dtPolyType trajType, const double t0,
                               const double tf, const ContainerType &p0,
                               const ContainerType &pf, const ContainerType &v0,
                               const ContainerType &vf, const ContainerType &a0,
                               const ContainerType &af);

private:
  dtPolyType _trajType;
  double _t0;
  double _tf;
#ifdef _DEBUG
  m_type _p0;
  m_type _pf;
  m_type _v0;
  m_type _vf;
  m_type _a0;
  m_type _af;
#endif
  dtMath::VectorXd _coeff;
};

} // namespace dtCore

#include "dtPolynomialTrajectory.tpp"

#endif // __DTCORE_DTPOLYNOMIALTRAJECTORY_H__