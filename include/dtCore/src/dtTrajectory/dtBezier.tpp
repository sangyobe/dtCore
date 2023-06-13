namespace dtCore {

template <typename ValueType, uint16_t m_degree>
dtBezier<ValueType, m_degree>::dtBezier() {
  static_assert(m_degree >= 1 && m_degree <= 3,
                "Invalid degree of bezier curve.");
}

template <typename ValueType, uint16_t m_degree>
dtBezier<ValueType, m_degree>::~dtBezier() {}

template <typename ValueType, uint16_t m_degree>
void dtBezier<ValueType, m_degree>::Interpolate(const ValueType t, ValueType &p,
                                                ValueType &v,
                                                ValueType &a) const {
  if (t < 0)
    p = m_p[0];
  else if (t > 1.0)
    p = m_p[m_degree];
  else {
    ValueType tmp = m_p[0];
    // for (uint16_t i = 0; i < m_degree + 1; i++) {
    //   tmp += m_p[i] * (1 - t)
    // }
    p = tmp;
  }
}

template <typename ValueType, uint16_t m_degree>
void dtBezier<ValueType, m_degree>::Configure(const ValueType p0,
                                              const ValueType pf,
                                              const ValueType *pc,
                                              ValueType duration) {
  m_duration = duration;
  m_p[0] = p0;
  memcpy(&m_p[1], pc, sizeof(ValueType) * m_degree);
  m_p[m_degree] = pf;
}

//////////////////////////////////////////////////////////////////////////////////////
// 1st order Bezier curve
//
template <typename ValueType> class dtBezier<ValueType, 1> {
public:
  dtBezier() {}
  virtual ~dtBezier() {}

public:
  void Interpolate(const ValueType t, ValueType &p, ValueType &v,
                   ValueType &a) const;

  void Configure(const ValueType p0, const ValueType pf, const ValueType *pc,
                 ValueType duration = 1.0);

private:
  ValueType m_duration;
  ValueType m_p[2];
};

template <typename ValueType>
void dtBezier<ValueType, 1>::Interpolate(const ValueType t, ValueType &p,
                                         ValueType &v, ValueType &a) const {
  if (t < 0)
    p = m_p[0];
  else if (t > m_duration)
    p = m_p[1];
  else {
    ValueType t_n = t / m_duration;
    ValueType t_c = 1 - t_n;
    p = t_c * m_p[0] + t_n * m_p[1];
  }

  v = 0;
  a = 0;
}

template <typename ValueType>
void dtBezier<ValueType, 1>::Configure(const ValueType p0, const ValueType pf,
                                       const ValueType *pc,
                                       ValueType duration) {

  m_duration = duration;
  m_p[0] = p0;
  m_p[1] = pf;
}

//////////////////////////////////////////////////////////////////////////////////////
// 2nd order Bezier curve
//
template <typename ValueType> class dtBezier<ValueType, 2> {
public:
  dtBezier() {}
  virtual ~dtBezier() {}

public:
  void Interpolate(const ValueType t, ValueType &p, ValueType &v,
                   ValueType &a) const;

  void Configure(const ValueType p0, const ValueType pf, const ValueType *pc,
                 ValueType duration = 1.0);

private:
  ValueType m_duration;
  ValueType m_p[3];
};

template <typename ValueType>
void dtBezier<ValueType, 2>::Interpolate(const ValueType t, ValueType &p,
                                         ValueType &v, ValueType &a) const {
  if (t < 0)
    p = m_p[0];
  else if (t > m_duration)
    p = m_p[2];
  else {
    ValueType t_n = t / m_duration;
    ValueType t_c = 1 - t_n;
    p = t_c * t_c * m_p[0] + 2 * t_c * t_n * m_p[1] + t_n * t_n * m_p[2];
  }

  v = 0;
  a = 0;
}

template <typename ValueType>
void dtBezier<ValueType, 2>::Configure(const ValueType p0, const ValueType pf,
                                       const ValueType *pc,
                                       ValueType duration) {

  m_duration = duration;
  m_p[0] = p0;
  m_p[1] = pc[0];
  m_p[2] = pf;
}

//////////////////////////////////////////////////////////////////////////////////////
// 3rd order Bezier curve
//
template <typename ValueType> class dtBezier<ValueType, 3> {
public:
  dtBezier() {}
  virtual ~dtBezier() {}

public:
  void Interpolate(const ValueType t, ValueType &p, ValueType &v,
                   ValueType &a) const;

  void Configure(const ValueType p0, const ValueType pf, const ValueType *pc,
                 ValueType duration = 1.0);

private:
  ValueType m_duration;
  ValueType m_p[4];
};

template <typename ValueType>
void dtBezier<ValueType, 3>::Interpolate(const ValueType t, ValueType &p,
                                         ValueType &v, ValueType &a) const {
  if (t < 0)
    p = m_p[0];
  else if (t > m_duration)
    p = m_p[3];
  else {
    ValueType t_n = t / m_duration;
    ValueType t_c = 1 - t_n;
    p = t_c * t_c * t_c * m_p[0] + 3 * t_c * t_c * t_n * m_p[1] +
        3 * t_c * t_n * t_n * m_p[2] + t_n * t_n * t_n * m_p[3];
  }

  v = 0;
  a = 0;
}

template <typename ValueType>
void dtBezier<ValueType, 3>::Configure(const ValueType p0, const ValueType pf,
                                       const ValueType *pc,
                                       ValueType duration) {

  m_duration = duration;
  m_p[0] = p0;
  m_p[1] = pc[0];
  m_p[2] = pc[1];
  m_p[3] = pf;
}

} // namespace dtCore