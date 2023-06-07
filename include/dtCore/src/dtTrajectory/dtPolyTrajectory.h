// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTPOLYTRAJECTORY_H__
#define __DTCORE_DTPOLYTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtPolyTrajectory provides trajectory interpolation for one joint
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
 * dtPolyTrajectory<double, 3, 5> traj(t0, tf, pi, pf, vi, vf, ai, af);
 * 
 * double tc = 3.0;
 * dtVector<double, 3> p, v, a;
 * traj.interpolate(tc, p, v, a);
 * 
 * \endcode
 */

// #include "dtTrajectory.h"

namespace dtCore {

template <typename m_type, dtTrajType m_trajType, uint32_t dof = 1>
class dtPolyTrajectory {
public:
  typedef Eigen::Matrix<m_type, dof * 3, 1> m_valueType;
  dtPolyTrajectory();
  dtPolyTrajectory(const double t0, const double tf, const m_valueType &initial,
                   const m_valueType &final);
  virtual ~dtPolyTrajectory();

public:
  virtual void interpolate(const double t, m_valueType &current) const;

protected:
  virtual void _determineCoeff(const m_valueType &initial,
                               const m_valueType &final);

private:
  double _t0;
  double _tf;
#ifdef _DEBUG
  m_valueType _p0;
  m_valueType _pf;
#endif
  dtMath::VectorXd _coeff;
};

} // namespace dtCore

#include "dtPolyTrajectory.tpp"

#endif // __DTCORE_DTPOLYTRAJECTORY_H__