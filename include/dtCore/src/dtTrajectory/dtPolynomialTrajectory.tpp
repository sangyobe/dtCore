namespace dtCore {

template <typename ValueType, uint32_t DOF>
dtPolynomialTrajectory<ValueType, DOF>::dtPolynomialTrajectory()
    : _trajType(dtTrajType::NONE), _t0(t0), _tf(tf), _coeff() {}

template <typename ValueType, uint32_t DOF>
dtPolynomialTrajectory<ValueType, DOF>::~dtPolynomialTrajectory() {}

template <typename ValueType, uint32_t DOF>
dtPolynomialTrajectory<ValueType, DOF>::dtPolynomialTrajectory(
    dtTrajType trajType, const double t0, const double tf,
    const ContainerType &p0, const ContainerType &pf, const ContainerType &v0,
    const ContainerType &vf, const ContainerType &a0, const ContainerType &af)
    : _trajType(trajType), _t0(t0), _tf(tf), _coeff() {
#ifdef _DEBUG
  _p0 = p0;
  _pf = pf;
  _v0 = v0;
  _vf = vf;
  _a0 = a0;
  _af = af;
#endif
  _determineCoeff(_trajType, _t0, _tf, p0, pf, v0, vf, a0, af);
}

template <typename ValueType, uint32_t DOF>
void dtPolynomialTrajectory<ValueType, DOF>::interpolate(
    const double t_, ContainerType &p, ContainerType &v,
    ContainerType &a) const {

  double t = t_;
  if (t > this->_tf)
    t = this->_tf;
  if (t < this->_t0)
    t = this->_t0;

  t -= this->_t0;

  static_assert(_trajType != dtTrajType::NONE);

  switch (_trajType) {
  case dtTrajType::LINEAR: {
    p[0] = _coeff[0] + _coeff[1] * t;
    v[0] = _coeff[1];
    a[0] = 0.0;
  } break;

  case dtTrajType::QUADRATIC: {
    double tsqr = t * t;

    p[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr;
    v[0] = _coeff[1] + 2 * _coeff[2] * t;
    a[0] = 2 * _coeff[2];
  } break;

  case dtTrajType::CUBIC: {
    double tsqr = t * t;
    double tcub = t * tsqr;

    p[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr + _coeff[3] * tcub;
    v[0] = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr;
    a[0] = 2 * _coeff[2] + 6 * _coeff[3] * t;
  } break;

  case dtTrajType::QUINTIC:
  case dtTrajType::JERK: {
    double tsqr = t * t;
    double tcub = t * tsqr;
    double tquad = t * tcub;
    double tquint = t * tquad;

    p[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr + _coeff[3] * tcub +
           _coeff[4] * tquad + _coeff[5] * tquint;
    v[0] = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr +
           4 * _coeff[4] * tcub + 5 * _coeff[5] * tquad;
    a[0] = 2 * _coeff[2] + 6 * _coeff[3] * t + 12 * _coeff[4] * tsqr +
           20 * _coeff[5] * tcub;
  } break;
  }
}

template <typename ValueType, uint32_t DOF>
void dtPolynomialTrajectory<ValueType, DOF>::_determineCoeff(
    dtTrajType trajType, const double t0, const double tf,
    const ContainerType &p0, const ContainerType &pf, const ContainerType &v0,
    const ContainerType &vf, const ContainerType &a0, const ContainerType &af) {
  double t = tf - t0;
  double t2 = t * t;
  double t3 = t * t2;
  double t4 = t * t3;
  double t5 = t * t4;

  switch (_trajType) {
  case dtTrajType::NONE:
    break;

  case dtTrajType::LINEAR: {
    dtMath::Matrix2d B;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(1, 0) = 1;
    B(1, 1) = t;

    _coeff.resize(2);
    _coeff[0] = p0[0];
    _coeff[1] = pf[0];

    B.solve(_coeff);
  } break;

  case dtTrajType::QUADRATIC: {
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
    _coeff[0] = p0[0];
    _coeff[1] = v0[0];
    _coeff[2] = pf[0];

    B.solve(_coeff);
  } break;

  case dtTrajType::CUBIC: {
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
    _coeff[0] = p0[0];
    _coeff[1] = v0[0];
    _coeff[2] = pf[0];
    _coeff[3] = vf[0];

    B.solve(_coeff);
  } break;

  case dtTrajType::QUINTIC: {
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
    _coeff[0] = p0[0];
    _coeff[1] = v0[0];
    _coeff[2] = a0[0];
    _coeff[3] = pf[0];
    _coeff[4] = vf[0];
    _coeff[5] = af[0];

    B.solve(_coeff);
  } break;

  case dtTrajType::JERK: {
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
    _coeff[0] = p0[0];
    _coeff[1] = v0[0];
    _coeff[2] = a0[0];
    _coeff[3] = pf[0];
    _coeff[4] = vf[0];
    _coeff[5] = af[0];

    // enforcing zero jerk condition
    _coeff[2] = 0.0;
    _coeff[5] = 0.0;

    B.solve(_coeff);
  } break;
  }
}

} // namespace dtCore
