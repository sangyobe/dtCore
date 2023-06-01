/* Copyright Hyundai Motor Company. All rights reserved.
 *
 * This library is commercial and cannot be redistributed, and/or modified
 * WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.
 */
// dtInterpolator.h: interface for the Interpolation class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __DTCORE_DTINTERPOLATOR_H__
#define __DTCORE_DTINTERPOLATOR_H__

#include "dtMath/dtMath.h"

namespace dtCore {
class dtInterpolator {
public:
  enum TYPE {
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

public:
  dtInterpolator();
  virtual ~dtInterpolator();

  dtInterpolator(const TYPE type, const double t0, const double tf,
                 const dtMath::Vector3d &initial,
                 const dtMath::Vector3d &final);

  void reconfigure(const dtInterpolator::TYPE type, const double t0,
                   const double tf, const dtMath::Vector3d &initial,
                   const dtMath::Vector3d &final);

  void interpolate(const double t, double &p, double &v, double &a) const;

private:
  void _determineCoeff(const dtMath::Vector3d &initial,
                       const dtMath::Vector3d &final /*, void* data = NULL*/);

private:
  dtInterpolator::TYPE _type;
  double _t0;
  double _tf;
  dtMath::VectorXd _coeff;
};

dtInterpolator::dtInterpolator()
    : _type(TYPE::NONE), _t0(0), _tf(0), _coeff() {}

dtInterpolator::~dtInterpolator() {}

dtInterpolator::dtInterpolator(const TYPE type, const double t0,
                               const double tf, const dtMath::Vector3d &initial,
                               const dtMath::Vector3d &final)
    : _type(type), _t0(t0), _tf(tf), _coeff() {
  _determineCoeff(initial, final);
}

void dtInterpolator::reconfigure(const dtCore::dtInterpolator::TYPE type,
                                 const double t0, const double tf,
                                 const dtMath::Vector3d &initial,
                                 const dtMath::Vector3d &final) {
  _type = type;
  _t0 = t0;
  _tf = tf;

  _determineCoeff(initial, final);
}

void dtInterpolator::interpolate(const double _t, double &p, double &v,
                                 double &a) const {

  double t = _t;
  if (t > _tf)
    t = _tf;
  if (t < _t0)
    t = _t0;

  t -= _t0;

  assert(_type != TYPE::NONE);

  switch (_type) {
  case TYPE::LINEAR: {
    p = _coeff[0] + _coeff[1] * t;
    v = _coeff[1];
    a = 0.0;
  } break;

  case TYPE::QUADRATIC: {
    double tsqr = t * t;

    p = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr;
    v = _coeff[1] + 2 * _coeff[2] * t;
    a = 2 * _coeff[2];
  } break;

  case TYPE::CUBIC: {
    double tsqr = t * t;
    double tcub = t * tsqr;

    p = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr + _coeff[3] * tcub;
    v = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr;
    a = 2 * _coeff[2] + 6 * _coeff[3] * t;
  } break;

  case TYPE::QUINTIC:
  case TYPE::JERK: {
    double tsqr = t * t;
    double tcub = t * tsqr;
    double tquad = t * tcub;
    double tquint = t * tquad;

    p = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr + _coeff[3] * tcub +
        _coeff[4] * tquad + _coeff[5] * tquint;
    v = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr +
        4 * _coeff[4] * tcub + 5 * _coeff[5] * tquad;
    a = 2 * _coeff[2] + 6 * _coeff[3] * t + 12 * _coeff[4] * tsqr +
        20 * _coeff[5] * tcub;
  } break;

    // 	case TYPE::LINEAR_PARABOLIC_BLEND:
    // 		{
    // 			double t_a = _coeff[8];	// acceleration period
    // 			if (t < t_a)
    // 			{
    // 				double s = t - _t0;
    // 				p = _coeff[0] + _coeff[1]*s + _coeff[2]*s*s;
    // 				v = _coeff[1] + 2*_coeff[2]*s;
    // 				a = 2*_coeff[2];
    // 			}
    // 			else if (t < _tf - t_a)
    // 			{
    // 				p = _coeff[3] + _coeff[4]*(t - t_a);
    // 				v = _coeff[4];
    // 				a = 0;
    // 			}
    // 			else
    // 			{
    // 				double s = _tf - t;
    // 				p = _coeff[5] + _coeff[6]*s + _coeff[7]*s*s;
    // 				v = _coeff[6] + 2*_coeff[7]*s;
    // 				a = 2*_coeff[7];
    // 			}
    // 		}
  }
}

void dtInterpolator::_determineCoeff(
    const dtMath::Vector3d &initial,
    const dtMath::Vector3d &final /*, void* data = NULL*/) {
  double t = _tf - _t0;
  double t2 = t * t;
  double t3 = t * t2;
  double t4 = t * t3;
  double t5 = t * t4;

  switch (_type) {
  case TYPE::NONE:
    break;

  case TYPE::LINEAR: {
    dtMath::Matrix2d B;
    // 			B(0, 0) = 1.0; B(0, 1) = _t0;
    // 			B(1, 0) = 1.0; B(1, 1) = _tf;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(1, 0) = 1;
    B(1, 1) = t;

    _coeff.resize(2);
    _coeff[0] = initial[0];
    _coeff[1] = final[0];

    B.solve(_coeff);
  } break;

  case TYPE::QUADRATIC: {
    dtMath::Matrix3d B;
    // 			B(0, 0) = 1.0; B(0, 1) = _t0;  B(0, 2) = t0sqr;
    // 			B(1, 0) = 0.0; B(1, 1) = 1.0; B(1, 2) = 2*_t0;
    // 			B(2, 0) = 1.0; B(2, 1) = _tf;  B(2, 2) = tfsqr;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(0, 2) = 0;
    B(1, 0) = 0;
    B(1, 1) = 1;
    B(1, 2) = 0;
    B(2, 0) = 1;
    B(2, 1) = t;
    B(2, 2) = t2;

    _coeff.resize(3);
    _coeff[0] = initial[0];
    _coeff[1] = initial[1];
    _coeff[2] = final[0];

    B.solve(_coeff);
  } break;

  case TYPE::CUBIC: {
    dtMath::Matrix4d B;
    // 			B(0, 0) = 1.0; B(0, 1) = _t0;  B(0, 2) = t0sqr; B(0, 3)
    // = t0cub; 			B(1, 0) = 0.0; B(1, 1) = 1.0; B(1, 2) =
    // 2*_t0;  B(1, 3) = 3*t0sqr; 			B(2, 0) = 1.0; B(2, 1) =
    // _tf; B(2, 2) = tfsqr; B(2, 3) = tfcub; 			B(3, 0) = 0.0;
    // B(3, 1) = 1.0; B(3, 2) = 2*_tf;  B(3, 3) = 3*tfsqr;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(0, 2) = 0;
    B(0, 3) = 0;
    B(1, 0) = 0;
    B(1, 1) = 1;
    B(1, 2) = 0;
    B(1, 3) = 0;
    B(2, 0) = 1;
    B(2, 1) = t;
    B(2, 2) = t2;
    B(2, 3) = t3;
    B(3, 0) = 0;
    B(3, 1) = 1;
    B(3, 2) = 2 * t;
    B(3, 3) = 3 * t2;

    _coeff.resize(4);
    _coeff.head<2>() = initial.head<2>();
    _coeff.tail<2>() = final.head<2>();

    B.solve(_coeff);
  } break;

  case TYPE::QUINTIC: {
    dtMath::Matrix6d B;
    // 			B(0, 0) = 1.0; B(0, 1) = _t0;  B(0, 2) = t0sqr; B(0, 3)
    // = t0cub; B(0, 4) = t0quad;   B(0, 5) = t0quint;
    // B(1, 0) = 0.0; B(1, 1) = 1.0; B(1, 2) = 2*_t0;  B(1, 3) = 3*t0sqr; B(1,
    // 4) = 4*t0cub;  B(1, 5) = 5*t0quad; 			B(2, 0) = 0.0;
    // B(2, 1) = 0.0; B(2, 2) = 2.0;   B(2, 3) = 6*_t0; B(2, 4) = 12*t0sqr; B(2,
    // 5) = 20*t0cub; 			B(3, 0) = 1.0; B(3, 1) = _tf; B(3, 2) =
    // tfsqr; B(3, 3) = tfcub;   B(3, 4) = tfquad;   B(3, 5) =
    // tfquint; 			B(4, 0) = 0.0; B(4, 1) = 1.0; B(4, 2) =
    // 2*_tf; B(4, 3) = 3*tfsqr; B(4, 4) = 4*tfcub;  B(4, 5) = 5*tfquad; B(5, 0)
    // = 0.0; B(5, 1) = 0.0; B(5, 2) = 2.0;   B(5, 3) = 6*_tf;    B(5, 4) =
    // 12*tfsqr; B(5, 5) = 20*tfcub;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(0, 2) = 0;
    B(0, 3) = 0;
    B(0, 4) = 0;
    B(0, 5) = 0;
    B(1, 0) = 0;
    B(1, 1) = 1;
    B(1, 2) = 0;
    B(1, 3) = 0;
    B(1, 4) = 0;
    B(1, 5) = 0;
    B(2, 0) = 0;
    B(2, 1) = 0;
    B(2, 2) = 2;
    B(2, 3) = 0;
    B(2, 4) = 0;
    B(2, 5) = 0;
    B(3, 0) = 1;
    B(3, 1) = t;
    B(3, 2) = t2;
    B(3, 3) = t3;
    B(3, 4) = t4;
    B(3, 5) = t5;
    B(4, 0) = 0;
    B(4, 1) = 1;
    B(4, 2) = 2 * t;
    B(4, 3) = 3 * t2;
    B(4, 4) = 4 * t3;
    B(4, 5) = 5 * t4;
    B(5, 0) = 0;
    B(5, 1) = 0;
    B(5, 2) = 2;
    B(5, 3) = 6 * t;
    B(5, 4) = 12 * t2;
    B(5, 5) = 20 * t3;

    _coeff.resize(6);
    _coeff.head<3>() = initial.head<3>();
    _coeff.tail<3>() = final.head<3>();

    B.solve(_coeff);
  } break;

  case TYPE::JERK: {
    dtMath::Matrix6d B;
    // 			B(0, 0) = 1.0; B(0, 1) = _t0;  B(0, 2) = t0sqr; B(0, 3)
    // = t0cub; B(0, 4) = t0quad;   B(0, 5) = t0quint;
    // B(1, 0) = 0.0; B(1, 1) = 1.0; B(1, 2) = 2*_t0;  B(1, 3) = 3*t0sqr; B(1,
    // 4) = 4*t0cub;  B(1, 5) = 5*t0quad;
    // 			//B(2, 0) = 0.0; B(2, 1) = 0.0; B(2, 2) = 2.0;   B(2, 3)
    // = 6*_t0; B(2, 4) = 12*t0sqr; B(2, 5) = 20*t0cub;
    // B(2, 0) = 0.0; B(2, 1) = 0.0; B(2, 2) = 0.0;   B(2, 3) = 6.0;     B(2, 4)
    // = 24*_t0;    B(2, 5) =
    // 60*t0sqr; 			B(3, 0) = 1.0; B(3, 1) = _tf;  B(3, 2) =
    // tfsqr; B(3, 3) = tfcub;   B(3, 4) = tfquad;   B(3, 5) = tfquint; B(4, 0)
    // = 0.0; B(4, 1) = 1.0; B(4, 2) = 2*_tf;  B(4, 3) = 3*tfsqr; B(4, 4) =
    // 4*tfcub; B(4, 5) = 5*tfquad;
    // 			//B(5, 0) = 0.0; B(5, 1) = 0.0; B(5, 2) = 2.0;   B(5, 3)
    // = 6*_tf; B(5, 4) = 12*tfsqr; B(5, 5) = 20*tfcub;
    // B(5, 0) = 0.0; B(5, 1) = 0.0; B(5, 2) = 0.0;   B(5, 3) = 6.0;     B(5, 4)
    // = 24*_tf;    B(5, 5) = 60*tfsqr;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(0, 2) = 0;
    B(0, 3) = 0;
    B(0, 4) = 0;
    B(0, 5) = 0;
    B(1, 0) = 0;
    B(1, 1) = 1;
    B(1, 2) = 0;
    B(1, 3) = 0;
    B(1, 4) = 0;
    B(1, 5) = 0;
    B(2, 0) = 0;
    B(2, 1) = 0;
    B(2, 2) = 0;
    B(2, 3) = 6;
    B(2, 4) = 0;
    B(2, 5) = 0;
    B(3, 0) = 1;
    B(3, 1) = t;
    B(3, 2) = t2;
    B(3, 3) = t3;
    B(3, 4) = t4;
    B(3, 5) = t5;
    B(4, 0) = 0;
    B(4, 1) = 1;
    B(4, 2) = 2 * t;
    B(4, 3) = 3 * t2;
    B(4, 4) = 4 * t3;
    B(4, 5) = 5 * t4;
    B(5, 0) = 0;
    B(5, 1) = 0;
    B(5, 2) = 0;
    B(5, 3) = 6;
    B(5, 4) = 24 * t;
    B(5, 5) = 60 * t2;

    _coeff.resize(6);
    _coeff.head<3>() = initial.head<3>();
    _coeff.tail<3>() = final.head<3>();

    // enforcing zero jerk condition
    _coeff[2] = 0.0;
    _coeff[5] = 0.0;

    B.solve(_coeff);
  } break;

    // 	case TYPE::LINEAR_PARABOLIC_BLEND:
    // 		{
    // 			LPBParm* parm = static_cast<LPBParm*>(data);
    // 			assert(parm->vmax > 0);
    // 			if (parm->amax > 0)
    // 			{
    // 				double ta = (parm->vmax - v0)/parm->amax;
    // 				if (parm->ta < 0 || ta > parm->ta)
    // 					parm->ta = ta;
    //
    // 				double td = (parm->vmax)
    // 			}
    //
    //
    // 			double t_a;
    // 			if (accDurationDefined())
    // 				_coeff[8] =
    // 		}
  }
}

} // namespace dtCore

// #include "dtInterpolator.tpp"

#endif // __DTCORE_DTINTERPOLATOR_H__
