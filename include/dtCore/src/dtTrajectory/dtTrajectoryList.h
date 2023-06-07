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
 * dtPolynomialTrajectory<double, 3> traj1(dtTrajType::CUBIC, t0, t1, pi, p1);
 * dtPolynomialTrajectory<double, 3> traj2(dtTrajType::CUBIC, t1, t2, p1, p2);
 * dtPolynomialTrajectory<double, 3> traj3(dtTrajType::CUBIC, t2, tf, p2, pf);
 * dtTrajectoryList<dtPolynomialTrajectory<double, 3>> trajList;
 * trajList.Add(traj1);
 * trajList.Add(traj2);
 * trajList.Add(traj3);
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
 * dtOrientationTrajectory<double> traj1(0.0, 0.5, R0, R1);
 * dtOrientationTrajectory<double> traj2(0.5, 2.0, R1, R2);
 * dtOrientationTrajectory<double> traj3(3.0, 5.0, R2, Rf);
 * dtTrajectoryList<dtOrientationTrajectory<double>> trajList;
 * trajList.Add(traj1);
 * trajList.Add(traj2);
 * trajList.Add(traj3);
 *
 * dtRotation<double> Rc;
 * trajList.Interpolate(3.5, Rc);
 * \endcode
 *
 * Task space trajectory interpolation example:
 * \code
 * #include <dtCore/dtTrajectory>
 *
 * dtHTransformTrajectory<double> traj1(0.0, 0.5, T0, T1);
 * dtHTransformTrajectory<double> traj2(0.5, 2.0, T1, T2);
 * dtHTransformTrajectory<double> traj3(3.0, 5.0, T2, Tf);
 * dtTrajectoryList<dtHTransformTrajectory<double>> trajList;
 * trajList.add(traj1);
 * trajList.add(traj2);
 * trajList.add(traj3);
 *
 * dtHTransform<double> Tc;
 * trajList.interpolate(3.5, Tc);
 * \endcode
 */

#include "dtTrajectory.h"
#include <list>

namespace dtCore {

template <typename T> class dtTrajectoryList {
public:
  dtTrajectoryList() {}
  virtual ~dtTrajectoryList() {}

  void add(const T &traj);
  void interpolate(double t, typename T::ContainerType &p,
                   typename T::ContainerType &v,
                   typename T::ContainerType &a) const;

private:
  std::list<T> trajList_;
};

} // namespace dtCore

#include "dtTrajectoryList.tpp"

#endif // __DTCORE_DTTRAJECTORYLIST_H__