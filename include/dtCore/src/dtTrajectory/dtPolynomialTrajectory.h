// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTPOLYNOMIALTRAJECTORY_H__
#define __DTCORE_DTPOLYNOMIALTRAJECTORY_H__

/** \defgroup dtCore
 *
 * This module provides ...
 *
 * \code
 * #include <dtCore/dtPolynomialTrajectory.h>
 * // initialize trajectory interpolator
 * double t0 = 0.0;
 * double tf = 10.0;
 * dtMath::Vector3d vi;
 * vi << 0.0, 0.0, 0.0;
 * dtMath::Vector3d vf;
 * vf << 5.0, 0.0, 0.0;
 * dtPolynomialTrajectory<dtMath::dtVector3d, dtTrajType::LINEAR> intp(t0, tf,
 * vi, vf);
 * // compute interpolated trajectory
 * dtMath::dtVector3d p;
 * intp.interpolate(5.0, p);
 * \endcode
 */

#include "dtTrajectory.h"

namespace dtCore {

template <typename ValueType, uint32_t DOF> class dtPolynomialTrajectory {
public:
  typedef Eigen::Matrix<ValueType, DOF, 1> ContainerType;

public:
  dtPolynomialTrajectory();
  dtPolynomialTrajectory(dtTrajType trajType, const double t0, const double tf,
                         const ContainerType &p0, const ContainerType &pf,
                         const ContainerType &v0 = ContrainerType.Zero(),
                         const ContainerType &vf = ContrainerType.Zero(),
                         const ContainerType &a0 = ContrainerType.Zero(),
                         const ContainerType &af = ContrainerType.Zero());
  virtual ~dtPolynomialTrajectory();

public:
  virtual void interpolate(const double t, ContainerType &p, ContainerType &v,
                           ContainerType &a) const;

protected:
  virtual void _determineCoeff(dtTrajType trajType, const double t0,
                               const double tf, const ContainerType &p0,
                               const ContainerType &pf, const ContainerType &v0,
                               const ContainerType &vf, const ContainerType &a0,
                               const ContainerType &af);

private:
  dtTrajType _trajType;
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