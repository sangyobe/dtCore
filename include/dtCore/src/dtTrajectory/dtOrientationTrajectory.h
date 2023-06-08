// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTORIENTATIONTRAJECTORY_H__
#define __DTCORE_DTORIENTATIONTRAJECTORY_H__

/** \defgroup dtTrajectory
 *
 * dtOrientationTrajectory provides orientation trajectory interpolation in task space.
 *
 * \code
 * #include <dtCore/dtTrajectory>
 * 
 * double ti = 0.0;
 * double tf = 10.0;
 * dtRotation<double> Ri, Rf;
 * R0 << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
 * Rf << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
 * dtOrientationTrajectory<double> traj(t0, tf, Ri, Rf);
 * 
 * double tc = 3.0;
 * dtRotation<double> Rc;
 * traj.interpolate(tc, Rc);
 * 
 * \endcode
 */

namespace dtCore {

template <typename ValueType> class dtOrientationTrajectory {
public:
    typedef Eigen::Matrix<ValueType, 3, 3> ContainerType;

  public:
    dtOrientationTrajectory();
    dtOrientationTrajectory(const double t0, const double tf,
                            const ContainerType &initial,
                            const ContainerType &final);
    virtual ~dtOrientationTrajectory();

  public:
    virtual void interpolate(const double t, ContainerType &current) const;

  protected:
    virtual void _determineCoeff(const ContainerType &initial,
                                 const ContainerType &final);
};

} // namespace dtCore

#include "dtOrientationTrajectory.tpp"

#endif // __DTCORE_DTORIENTATIONTRAJECTORY_H__