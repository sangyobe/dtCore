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
 * dtPolyTrajectory<double, 6, 3> traj1(0.0, 0.5, p0, p1, v0, v1);
 * dtPolyTrajectory<double, 6, 3> traj2(0.5, 2.0, p1, p2, v1, v2);
 * dtPolyTrajectory<double, 6, 3> traj3(3.0, 5.0, p2, pf, v2, vf);
 * dtTrajectoryList<dtPolyTrajectory<double, 6, 3>> trajList;
 * trajList.Add(traj1);
 * trajList.Add(traj2);
 * trajList.Add(traj3);
 * 
 * dtVector<double, 3, 1> q, qdot, qddot;
 * trajList.Interpolate(3.5, q, qdot, qddot);
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
 * trajList.Add(traj1);
 * trajList.Add(traj2);
 * trajList.Add(traj3);
 * 
 * dtHTransform<double> Tc;
 * trajList.Interpolate(3.5, Tc);
 * \endcode
 */

#include "dtTrajectory.h"
#include <list>

namespace dtCore {

template <typename m_type, dtTrajType m_trajType> class dtTrajectoryList {
public:
  dtTrajectoryList() {}
  virtual ~dtTrajectoryList() {}

  void Add(const dtTrajectory<m_type, m_trajType> &traj);
  void Interpolate(double t, m_type& current) const;

private:
  std::list<dtTrajectory<m_type, m_trajType>> trajList_;
};

} // namespace dtCore

#endif // __DTCORE_DTTRAJECTORYLIST_H__