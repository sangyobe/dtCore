/* Copyright Hyundai Motor Company. All rights reserved.
 *
 * This library is commercial and cannot be redistributed, and/or modified
 * WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.
 */
// dtMath.h:
//
//////////////////////////////////////////////////////////////////////

#ifndef __DTMATH_DTMATH_H__
#define __DTMATH_DTMATH_H__

#include <vector>
// using namespace std;
#define EIGEN_MATRIXBASE_PLUGIN "dtMath/plugin/EigenMatrixBasePlugin.h"
// #define EIGEN_MATRIX_PLUGIN    "dtMath/plugin/EigenMatrixPlugin.h"
// #define EIGEN_TRANSFORM_PLUGIN "dtMath/plugin/EigenTransformPlugin.h"
// #include <Eigen/Eigen>
#include <Eigen/Dense>

namespace dtMath {
typedef Eigen::Vector2d Vector2d;
typedef Eigen::Vector3d Vector3d;
typedef Eigen::Vector4d Vector4d;
typedef Eigen::Matrix<double, 6, 1> Vector6d;
typedef Eigen::VectorXd VectorXd;
typedef Eigen::Matrix2d Matrix2d;
typedef Eigen::Matrix3d Matrix3d;
typedef Eigen::Matrix4d Matrix4d;
typedef Eigen::Matrix<double, 6, 6> Matrix6d;
typedef Eigen::MatrixXd MatrixXd;
typedef Eigen::Matrix3d Rotation;
typedef Eigen::Matrix4d HTransform;
} // namespace dtMath

#endif // __DTMATH_DTMATH_H__
