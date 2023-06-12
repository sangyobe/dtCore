namespace dtCore {

template <typename ValueType, uint16_t m_degree>
dtPolynomialInterpolator<ValueType, m_degree>::dtPolynomialInterpolator(
    ValueType duration, ValueType p0, ValueType pf, ValueType v0, ValueType vf,
    ValueType a0, ValueType af)
    : dtInterpolator<ValueType>(), m_duration(duration) {}

template <typename ValueType, uint16_t m_degree>
dtPolynomialInterpolator<ValueType, m_degree>::~dtPolynomialInterpolator() {}

template <typename ValueType, uint16_t m_degree>
void dtPolynomialInterpolator<ValueType, m_degree>::Interpolate(
    ValueType t, ValueType &p, ValueType &v, ValueType &a) const {}

template <typename ValueType, uint16_t m_degree>
void dtPolynomialInterpolator<ValueType, m_degree>::_determinCoeff() {}

template <typename ValueType, uint16_t m_degree>
dtBezierInterpolator<ValueType, m_degree>::dtBezierInterpolator(
    ValueType duration, ValueType p0, ValueType pf, ValueType *tc,
    ValueType *pc)
    : dtInterpolator<ValueType>() {
  static_assert(m_degree >= 2 && m_degree <= 3,
                "Invalide degree of bezier curve.");

  m_tc[0] = 0;
  m_pc[0] = p0;
  m_tc[m_degree] = duration;
  m_pc[m_degree] = pf;
  memcpy(&m_tc[1], tc, sizeof(ValueType) * m_degree);
  memcpy(&m_pc[1], pc);

#ifdef _DEBUG
  for (uint16_t i = 0; i < m_degree; i++) {
    static_assert(m_tc[i] < m_tc[i + 1], "Time-asix dis-order.");
  }
#endif
}

template <typename ValueType, uint16_t m_degree>
dtBezierInterpolator<ValueType, m_degree>::~dtBezierInterpolator() {}

template <typename ValueType, uint16_t m_degree>
void dtBezierInterpolator<ValueType, m_degree>::Interpolate(
    ValueType t, ValueType &p, ValueType &v, ValueType &a) const {
  if (t < 0)
    p = m_pc[0];
  else if (t > m_duration)
    p = m_pc[m_degree];
  else {
    ValueType tmp = 0;
    // for (uint16_t i = 0; i < m_degree + 1; i++) {
    //   tmp += m_pc[i] * (1 - t)
    // }
  }
#ifdef _DEBUG
  v = static_cast<ValueType> 0;
  a = static_cast<ValueType> 0;
#endif
}

template <typename ValueType>
dtLinearParabolicBlendInterpolator<
    ValueType>::~dtLinearParabolicBlendInterpolator() {}

template <typename ValueType>
void dtLinearParabolicBlendInterpolator<ValueType>::Interpolate(
    ValueType t, ValueType &p, ValueType &v, ValueType &a) const {}

} // namespace dtCore