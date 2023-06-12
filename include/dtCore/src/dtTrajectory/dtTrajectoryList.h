// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTTRAJECTORYLIST_H__
#define __DTCORE_DTTRAJECTORYLIST_H__

/** \defgroup dtTrajectory
 *
 * dtTrajectoryList provides trajectory interpolation based on
 * a list of trajectories consist of a starting point, multiple via points,
 * and a final point.
 *
 * N-DOF Joint trajectory interpolation example:
 * \code
 * #include <dtCore/dtTrajectory>
 *
 * dtPolynomialTrajectory<double, 3> traj1(dtPolyType::CUBIC, ti, t1, pi, p1);
 * dtPolynomialTrajectory<double, 3> traj2(dtPolyType::CUBIC, t1, t2, p1, p2);
 * dtPolynomialTrajectory<double, 3> traj3(dtPolyType::CUBIC, t2, tf, p2, pf);
 * dtTrajectoryList<dtPolynomialTrajectory<double, 3>> trajList;
 * trajList.add(traj1);
 * trajList.add(traj2);
 * trajList.add(traj3);
 *
 * double tc = 3.0;
 * dtVector<double, 3> p, v, a;
 * trajList.interpolate(tc, p, v, a);
 *
 * \endcode
 *
 * Orientation trajectory interpolation example:
 * \code
 * #include <dtCore/dtTrajectory>
 *
 * dtMath::Rotation<double> Ri, R1, R2, Rf;
 * Ri << 1.0, ...;
 * R1 << 1.0, ...;
 * R2 << 1.0, ...;
 * Rf << 1.0, ...;
 * dtOrientationTrajectory<double> traj1(0.0, 0.5, Ri, R1);
 * dtOrientationTrajectory<double> traj2(0.5, 2.0, R1, R2);
 * dtOrientationTrajectory<double> traj3(3.0, 5.0, R2, Rf);
 * dtTrajectoryList<dtOrientationTrajectory<double>> trajList;
 * trajList.add(traj1);
 * trajList.add(traj2);
 * trajList.add(traj3);
 *
 * dtMath::Rotation<double> Rc;
 * trajList.interpolate(3.5, Rc);
 * \endcode
 *
 * Task space trajectory interpolation example:
 * \code
 * #include <dtCore/dtTrajectory>
 *
 * dtMath::HTransform<double> T0, T1, T2, Tf;
 * Ti << 1.0, ...;
 * T1 << 1.0, ...;
 * T2 << 1.0, ...;
 * Tf << 1.0, ...;
 * dtHTransformTrajectory<double> traj1(0.0, 0.5, Ti, T1);
 * dtHTransformTrajectory<double> traj2(0.5, 2.0, T1, T2);
 * dtHTransformTrajectory<double> traj3(3.0, 5.0, T2, Tf);
 * dtTrajectoryList<dtHTransformTrajectory<double>> trajList;
 * trajList.add(traj1);
 * trajList.add(traj2);
 * trajList.add(traj3);
 *
 * dtMath::HTransform<double> Tc;
 * trajList.interpolate(3.5, Tc);
 * \endcode
 */

#include "dtMath/dtMath.h"
#include <list>

namespace dtCore {

template <typename TrajType> class dtTrajectoryList {
public:
  dtTrajectoryList() {}
  virtual ~dtTrajectoryList() {}

  void add(const TrajType &traj);
  void interpolate(double t, typename TrajType::ContainerType &p,
                   typename TrajType::ContainerType &v,
                   typename TrajType::ContainerType &a) const;
  void interpolate(double t, typename TrajType::ContainerType &p) const;

private:
  std::list<TrajType> trajList_;
};

} // namespace dtCore

#include "dtTrajectoryList.tpp"

#endif // __DTCORE_DTTRAJECTORYLIST_H__