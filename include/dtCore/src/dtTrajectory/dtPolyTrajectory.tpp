namespace dtCore {

template <typename m_type, dtTrajType m_trajType, uint32_t dof>
dtPolyTrajectory<m_type, m_trajType, dof>::dtPolyTrajectory()
    : _t0(0), _tf(0), _coeff() {}

template <typename m_type, dtTrajType m_trajType, uint32_t dof>
dtPolyTrajectory<m_type, m_trajType, dof>::~dtPolyTrajectory() {}

template <typename m_type, dtTrajType m_trajType, uint32_t dof>
dtPolyTrajectory<m_type, m_trajType, dof>::dtPolyTrajectory(
    const double t0, const double tf, const m_valueType &initial,
    const m_valueType &final)
    : _t0(t0), _tf(tf), _coeff() {
#ifdef _DEBUG
  _p0 = initial;
  _pf = final;
#endif
  _determineCoeff(initial, final);
}

template <typename m_type, dtTrajType m_trajType, uint32_t dof>
void dtPolyTrajectory<m_type, m_trajType, dof>::interpolate(
    const double t_, m_valueType &current) const {

  double t = t_;
  if (t > this->_tf)
    t = this->_tf;
  if (t < this->_t0)
    t = this->_t0;

  t -= this->_t0;

  static_assert(m_trajType != dtTrajType::NONE);

  switch (m_trajType) {
  case dtTrajType::LINEAR: {
    current[0] = _coeff[0] + _coeff[1] * t;
    current[1] = _coeff[1];
    current[2] = 0.0;
  } break;

  case dtTrajType::QUADRATIC: {
    double tsqr = t * t;

    current[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr;
    current[1] = _coeff[1] + 2 * _coeff[2] * t;
    current[2] = 2 * _coeff[2];
  } break;

  case dtTrajType::CUBIC: {
    double tsqr = t * t;
    double tcub = t * tsqr;

    current[0] =
        _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr + _coeff[3] * tcub;
    current[1] = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr;
    current[2] = 2 * _coeff[2] + 6 * _coeff[3] * t;
  } break;

  case dtTrajType::QUINTIC:
  case dtTrajType::JERK: {
    double tsqr = t * t;
    double tcub = t * tsqr;
    double tquad = t * tcub;
    double tquint = t * tquad;

    current[0] = _coeff[0] + _coeff[1] * t + _coeff[2] * tsqr +
                 _coeff[3] * tcub + _coeff[4] * tquad + _coeff[5] * tquint;
    current[1] = _coeff[1] + 2 * _coeff[2] * t + 3 * _coeff[3] * tsqr +
                 4 * _coeff[4] * tcub + 5 * _coeff[5] * tquad;
    current[2] = 2 * _coeff[2] + 6 * _coeff[3] * t + 12 * _coeff[4] * tsqr +
                 20 * _coeff[5] * tcub;
  } break;
  }
}

template <typename m_type, dtTrajType m_trajType, uint32_t dof>
void dtPolyTrajectory<m_type, m_trajType, dof>::_determineCoeff(
    const m_valueType &initial, const m_valueType &final) {
  double t = this->_tf - this->_t0;
  double t2 = t * t;
  double t3 = t * t2;
  double t4 = t * t3;
  double t5 = t * t4;

  switch (m_trajType) {
  case dtTrajType::NONE:
    break;

  case dtTrajType::LINEAR: {
    dtMath::Matrix2d B;
    B(0, 0) = 1;
    B(0, 1) = 0;
    B(1, 0) = 1;
    B(1, 1) = t;

    _coeff.resize(2);
    _coeff[0] = initial[0];
    _coeff[1] = final[0];

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
    _coeff[0] = initial[0];
    _coeff[1] = initial[1];
    _coeff[2] = final[0];

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
    _coeff.head<2>() = initial.template head<2>();
    _coeff.tail<2>() = final.template head<2>();

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
    _coeff.head<3>() = initial.template head<3>();
    _coeff.tail<3>() = final.template head<3>();

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
    _coeff.head<3>() = initial.template head<3>();
    _coeff.tail<3>() = final.template head<3>();

    // enforcing zero jerk condition
    _coeff[2] = 0.0;
    _coeff[5] = 0.0;

    B.solve(_coeff);
  } break;
  }
}

} // namespace dtCore
