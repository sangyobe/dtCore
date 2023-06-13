namespace dtCore {

////////////////////////////////////////////////////////////////////////////////
// Implementation of dtBezierTrajectory
//
template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
dtBezierTrajectory<ValueType, m_dof, m_degree>::dtBezierTrajectory(
    const ValueType duration, const ContRefType pi, const ContRefType pf,
    const ContRefType pc, const ValueType timeOffset)
    : m_duration(duration), m_ti(timeOffset), m_tf(timeOffset + duration) {
  static_assert(m_degree > 0, "Invalid degree of bezier.");

  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  if (pc && m_degree > 1) {
    memcpy(m_pc, pc, sizeof(ValueType) * m_dof * (m_degree - 1));
  }

  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
dtBezierTrajectory<ValueType, m_dof, m_degree>::~dtBezierTrajectory() {}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtBezierTrajectory<ValueType, m_dof, m_degree>::Interpolate(
    const ValueType t, ContRefType p, ContRefType v, ContRefType a) const {
  ValueType t_ = t - this->m_ti;
  if (t_ < 0) {
    memcpy(p, this->m_pi, sizeof(ValueType) * m_dof);
    // memcpy(v, m_vi, sizeof(ValueType) * m_dof);
    //  memcpy(a, m_ai, sizeof(ValueType) * m_dof);
    memset(v, 0, sizeof(ValueType) * m_dof);
    memset(a, 0, sizeof(ValueType) * m_dof);
  } else if (t_ > this->m_duration) {
    memcpy(p, this->m_pf, sizeof(ValueType) * m_dof);
    // memcpy(v, m_vf, sizeof(ValueType) * m_dof);
    // memcpy(a, m_af, sizeof(ValueType) * m_dof);
    memset(v, 0, sizeof(ValueType) * m_dof);
    memset(a, 0, sizeof(ValueType) * m_dof);
  } else {
    for (uint16_t i = 0; i < m_dof; i++) {
      m_interpolator[i].Interpolate(t_, p[i], v[i], a[i]);
    }
  }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtBezierTrajectory<ValueType, m_dof, m_degree>::Reconfigure() {
  for (uint16_t i = 0; i < m_dof; i++) {
    m_interpolator[i].Configure(this->m_pi[i], this->m_pf[i], this->m_pc[i],
                                this->m_duration);
  }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtBezierTrajectory<ValueType, m_dof, m_degree>::SetTimeOffset(
    const ValueType timeOffset) {
  m_ti = timeOffset;
  m_tf = m_ti + m_duration;
  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtBezierTrajectory<ValueType, m_dof, m_degree>::SetDuration(
    const ValueType duration) {
  m_duration = duration;
  m_tf = m_ti + m_duration;
  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtBezierTrajectory<ValueType, m_dof, m_degree>::SetInitialParam(
    const ContRefType pi) {
  if (pi)
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtBezierTrajectory<ValueType, m_dof, m_degree>::SetTargetParam(
    const ContRefType pf) {
  if (pf)
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_degree>
void dtBezierTrajectory<ValueType, m_dof, m_degree>::SetControlParam(
    const ContRefType pc) {
  if (pc)
    memcpy(m_pc, pc, sizeof(ValueType) * m_dof * (m_degree - 1));
  Reconfigure();
}

} // namespace dtCore