namespace dtCore {

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
dtTrajectory<ValueType, m_dof, m_degree>::dtTrajectory(
    const ValueType duration, const ContRefType pi, const ContRefType pf,
    const ContRefType vi, const ContRefType vf, const ContRefType ai,
    const ContRefType af, const ValueType timeOffset)
    : m_duration(duration), m_ti(timeOffset), m_tf(timeOffset + duration) {

  static_assert(m_degree == 1 || m_degree == 3 || m_degree == 5 ||
                    m_degree == 7,
                "Invalid degree of polynomial.");

  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
  memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
  memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
  memcpy(m_af, af, sizeof(ValueType) * m_dof);

  ReconfigurePolynomial();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
dtTrajectory<ValueType, m_dof, m_degree>::~dtTrajectory() {}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::Interpolate(
    const ValueType t, ContRefType p, ContRefType v, ContRefType a) const {
  if (t < m_ti) {
    memcpy(p, m_pi, sizeof(ValueType) * m_dof);
    // memcpy(v, m_vi, sizeof(ValueType) * m_dof);
    //  memcpy(a, m_ai, sizeof(ValueType) * m_dof);
    memset(v, 0, sizeof(ValueType) * m_dof);
    memset(a, 0, sizeof(ValueType) * m_dof);
    return;
  } else if (t > m_tf) {
    memcpy(p, m_pf, sizeof(ValueType) * m_dof);
    // memcpy(v, m_vf, sizeof(ValueType) * m_dof);
    // memcpy(a, m_af, sizeof(ValueType) * m_dof);
    memset(v, 0, sizeof(ValueType) * m_dof);
    memset(a, 0, sizeof(ValueType) * m_dof);
    return;
  }

  for (uint16_t i = 0; i < m_dof; i++) {
    m_poly[i].Interpolate(t - m_ti, p[i], v[i], a[i]);
  }
}
template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::SetTimeOffset(
    const ValueType timeOffset) {
  m_ti = timeOffset;
  m_tf = m_ti + m_duration;
  ReconfigurePolynomial();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::SetDuration(
    const ValueType duration) {
  m_duration = duration;
  m_tf = m_ti + m_duration;
  ReconfigurePolynomial();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::SetInitialParam(
    const ContRefType pi, const ContRefType vi, const ContRefType ai) {
  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
  memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
  ReconfigurePolynomial();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::SetTargetParam(
    const ContRefType pf, const ContRefType vf, const ContRefType af) {
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
  memcpy(m_af, af, sizeof(ValueType) * m_dof);
  ReconfigurePolynomial();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::ReconfigurePolynomial() {
  for (uint16_t i = 0; i < m_dof; i++) {
    m_poly[i].DetermineCoeff(m_duration, m_pi[i], m_pf[i], m_vi[i], m_vf[i],
                             m_ai[i], m_af[i]);
  }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
dtTrajectory<ValueType, m_dof, m_degree>::dtPolynomial::dtPolynomial() {}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
dtTrajectory<ValueType, m_dof, m_degree>::dtPolynomial::~dtPolynomial() {}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::dtPolynomial::Interpolate(
    const ValueType t, ValueType &p, ValueType &v, ValueType &a) const {
  if (t < 0) {
    p = m_p0;
    v = a = 0;
    return;
  } else if (t > m_duration) {
    p = m_pf;
    v = a = 0;
    return;
  }

  switch (m_degree) {
  case 1: {
    p = m_coeff[0] + m_coeff[1] * t;
    v = m_coeff[1];
    a = 0.0;
  } break;

  case 3: {
    ValueType t2 = t * t;
    p = m_coeff[0] + m_coeff[1] * t + m_coeff[2] * t2;
    v = m_coeff[1] + 2 * m_coeff[2] * t;
    a = 2 * m_coeff[2];
  } break;

  case 5: {
    ValueType t2 = t * t;
    ValueType t3 = t * t2;
    ValueType t4 = t * t3;
    ValueType t5 = t * t4;
    p = m_coeff[0] + m_coeff[1] * t + m_coeff[2] * t2 + m_coeff[3] * t3 +
        m_coeff[4] * t4 + m_coeff[5] * t5;
    v = m_coeff[1] + 2 * m_coeff[2] * t + 3 * m_coeff[3] * t2 +
        4 * m_coeff[4] * t3 + 5 * m_coeff[5] * t4;
    a = 2 * m_coeff[2] + 6 * m_coeff[3] * t + 12 * m_coeff[4] * t2 +
        20 * m_coeff[5] * t3;
  } break;

  case 7:
    // break;
  default:
    // assert(false, "Invalid degree of polynomial.");
    break;
  }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtTrajectory<ValueType, m_dof, m_degree>::dtPolynomial::DetermineCoeff(
    const ValueType duration, const ValueType &p0, const ValueType &pf,
    const ValueType &v0, const ValueType &vf, const ValueType &a0,
    const ValueType &af) {
  m_duration = duration;
  m_p0 = p0;
  m_pf = pf;

  switch (m_degree) {
  case 1:
    m_coeff[0] = p0;
    m_coeff[1] = (pf - p0) / duration;
    break;
  case 3:
    m_coeff[0] = 0;
    m_coeff[1] = 0;
    m_coeff[2] = 0;
    break;
  case 5:
    m_coeff[0] = 0;
    m_coeff[1] = 0;
    m_coeff[2] = 0;
    m_coeff[3] = 0;
    m_coeff[4] = 0;
    m_coeff[5] = 0;
    break;
  case 7:
    // break;
  default:
    // assert(false, "Invalid degree of polynomial.");
    break;
  }
}

} // namespace dtCore