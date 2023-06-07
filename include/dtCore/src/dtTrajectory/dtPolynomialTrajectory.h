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

template <typename m_type, dtTrajType m_trajType>
class dtPolynomialTrajectory : public dtTrajectory<m_type, m_trajType> {
public:
  dtPolynomialTrajectory();
  dtPolynomialTrajectory(const double t0, const double tf,
                         const m_type &initial, const m_type &final);
  virtual ~dtPolynomialTrajectory();

public:
  virtual void interpolate(const double t, m_type &current) const override;

protected:
  virtual void _determineCoeff(const m_type &initial,
                               const m_type &final) override;

private:
  dtMath::VectorXd _coeff;
};

} // namespace dtCore

#include "dtPolynomialTrajectory.tpp"

#endif // __DTCORE_DTPOLYNOMIALTRAJECTORY_H__