namespace dtCore {

template <typename ValueType, uint32_t DOF>
dtVectorPolynomialTrajectory<ValueType, DOF>::dtVectorPolynomialTrajectory()
    : _trajType(dtPolyType::NONE), _t0(0), _tf(0), _coeff() {}

template <typename ValueType, uint32_t DOF>
dtVectorPolynomialTrajectory<ValueType, DOF>::~dtVectorPolynomialTrajectory() {}

template <typename ValueType, uint32_t DOF>
dtVectorPolynomialTrajectory<ValueType, DOF>::dtVectorPolynomialTrajectory(
    dtPolyType trajType, const ValueType t0, const ValueType tf,
    const ContRefType p0, const ContRefType pf)
    : _trajType(trajType), _t0(t0), _tf(tf), _coeff() {
  _p0 = p0;
  _pf = pf;
  _v0 = ContType::Zero();
  _vf = ContType::Zero();
  _a0 = ContType::Zero();
  _af = ContType::Zero();
  Reconfigure();
}

template <typename ValueType, uint32_t DOF>
dtVectorPolynomialTrajectory<ValueType, DOF>::dtVectorPolynomialTrajectory(
    dtPolyType trajType, const ValueType t0, const ValueType tf,
    const ContRefType p0, const ContRefType pf, const ContRefType v0,
    const ContRefType vf)
    : _trajType(trajType), _t0(t0), _tf(tf), _coeff() {
  _p0 = p0;
  _pf = pf;
  _v0 = v0;
  _vf = vf;
  _a0 = ContType::Zero();
  _af = ContType::Zero();
  Reconfigure();
}

template <typename ValueType, uint32_t DOF>
dtVectorPolynomialTrajectory<ValueType, DOF>::dtVectorPolynomialTrajectory(
    dtPolyType trajType, const ValueType t0, const ValueType tf,
    const ContRefType p0, const ContRefType pf, const ContRefType v0,
    const ContRefType vf, const ContRefType a0, const ContRefType af)
    : _trajType(trajType), _t0(t0), _tf(tf), _coeff() {
  _p0 = p0;
  _pf = pf;
  _v0 = v0;
  _vf = vf;
  _a0 = a0;
  _af = af;
  Reconfigure();
}

template <typename ValueType, uint32_t DOF>
void dtVectorPolynomialTrajectory<ValueType, DOF>::Interpolate(
    const ValueType t_, ContRefType p, ContRefType v, ContRefType a) const {

  ValueType t = t_;
  if (t > this->_tf)
    t = this->_tf;
  if (t < this->_t0)
    t = this->_t0;

  t -= this->_t0;

  assert(_trajType != dtPolyType::NONE);

  switch (_trajType) {
  case dtPolyType::LINEAR: {
    p[0] = _coeff[0] + _coeff[1] * t;
    v[0] = _coeff[1];
    a[0] = 0.0;
  } break;

  case dtPolyType::QUADRATIC: {
    ValueType tsqr = t * t;

    p[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr;
    v[0] = _coeff[1] + 2 * _coeff[2] * t;
    a[0] = 2 * _coeff[2];
  } break;

  case dtPolyType::CUBIC: {
    ValueType tsqr = t * t;
    ValueType tcub = t * tsqr;

    p[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr + _coeff[3] * tcub;
    v[0] = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr;
    a[0] = 2 * _coeff[2] + 6 * _coeff[3] * t;
  } break;

  case dtPolyType::QUINTIC:
  case dtPolyType::JERK: {
    ValueType tsqr = t * t;
    ValueType tcub = t * tsqr;
    ValueType tquad = t * tcub;
    ValueType tquint = t * tquad;

    p[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr + _coeff[3] * tcub +
           _coeff[4] * tquad + _coeff[5] * tquint;
    v[0] = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr +
           4 * _coeff[4] * tcub + 5 * _coeff[5] * tquad;
    a[0] = 2 * _coeff[2] + 6 * _coeff[3] * t + 12 * _coeff[4] * tsqr +
           20 * _coeff[5] * tcub;
  } break;

  case dtPolyType::NONE:
  default:
    break;
  }
}

template <typename ValueType, uint32_t DOF>
void dtVectorPolynomialTrajectory<ValueType, DOF>::Reconfigure() {
  ValueType t = _tf - _t0;
  ValueType t2 = t * t;
  ValueType t3 = t * t2;
  ValueType t4 = t * t3;
  ValueType t5 = t * t4;

  switch (_trajType) {
  case dtPolyType::LINEAR: {
    dtMath::Matrix2d B;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(1, 0) = 1;
    B(1, 1) = t;

    _coeff.resize(2);
    _coeff[0] = _p0[0];
    _coeff[1] = _pf[0];

    B.solve(_coeff);
  } break;

  case dtPolyType::QUADRATIC: {
    dtMath::Matrix3d B;
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
    _coeff[0] = _p0[0];
    _coeff[1] = _v0[0];
    _coeff[2] = _pf[0];

    B.solve(_coeff);
  } break;

  case dtPolyType::CUBIC: {
    dtMath::Matrix4d B;
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
    _coeff[0] = _p0[0];
    _coeff[1] = _v0[0];
    _coeff[2] = _pf[0];
    _coeff[3] = _vf[0];

    B.solve(_coeff);
  } break;

  case dtPolyType::QUINTIC: {
    dtMath::Matrix6d B;
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
    _coeff[0] = _p0[0];
    _coeff[1] = _v0[0];
    _coeff[2] = _a0[0];
    _coeff[3] = _pf[0];
    _coeff[4] = _vf[0];
    _coeff[5] = _af[0];

    B.solve(_coeff);
  } break;

  case dtPolyType::JERK: {
    dtMath::Matrix6d B;
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
    _coeff[0] = _p0[0];
    _coeff[1] = _v0[0];
    _coeff[2] = _a0[0];
    _coeff[3] = _pf[0];
    _coeff[4] = _vf[0];
    _coeff[5] = _af[0];

    // enforcing zero jerk condition
    _coeff[2] = 0.0;
    _coeff[5] = 0.0;

    B.solve(_coeff);
  } break;

  case dtPolyType::NONE:
  default:
    break;
  }
}

} // namespace dtCore
