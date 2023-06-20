namespace dtCore {

template <typename ValueType, uint16_t m_order>
dtBezier<ValueType, m_order>::dtBezier() 
{
  static_assert(m_order >= 1, "Invalid degree of bezier curve.");
}

template <typename ValueType, uint16_t m_order>
dtBezier<ValueType, m_order>::~dtBezier() {}

template <typename ValueType, uint16_t m_order>
void dtBezier<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p) const 
{
    const ValueType ctrlParam = t / m_duration;
    ValueType pos = 0;
    
    for (int i = 0; i < m_num; i++)
    {
        pos += BernsteinPoly(m_num - 1, i, ctrlParam) * m_p[i];
    }

    p = pos;
}

template <typename ValueType, uint16_t m_order>
void dtBezier<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p, ValueType &v) const 
{
    const ValueType ctrlParam = t / m_duration;
    ValueType pos = 0;
    ValueType vel = 0;
    
    for (int i = 0; i < m_num; i++)
    {
      pos += BernsteinPoly(m_num - 1, i, ctrlParam) * m_p[i];
      if (i < m_num - 1)
      {
          vel += (m_num - 1) * BernsteinPoly(m_num - 2, i, ctrlParam) * (m_p[i + 1] - m_p[i]); 
      }
    }

    p = pos;
    v = vel / m_duration;
}

template <typename ValueType, uint16_t m_order>
void dtBezier<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const 
{
    const ValueType ctrlParam = t / m_duration;
    ValueType pos = 0;
    ValueType vel = 0;
    ValueType acc = 0;
    
    for (int i = 0; i < m_num; i++)
    {
      pos += BernsteinPoly(m_num - 1, i, ctrlParam) * m_p[i];
      if (i < m_num - 1)
      {
          vel += (m_num - 1) * BernsteinPoly(m_num - 2, i, ctrlParam) * (m_p[i + 1] - m_p[i]); 
      }
      if (i < m_num - 2)
      {
          acc += (m_num - 1) * (m_num - 2) * BernsteinPoly(m_num - 3, i, ctrlParam) * ((m_p[i + 2] - m_p[i + 1]) - (m_p[i + 1] - m_p[i])); 
      }
    }

    p = pos;
    v = vel / m_duration;
    a = acc / (m_duration*m_duration);
}

template <typename ValueType, uint16_t m_order>
ValueType dtBezier<ValueType, m_order>::BernsteinPoly(int n, int k, ValueType ctrlParm) const
{
  return BinomialCoeff(n, k) * pow(ctrlParm, k) * pow(1 - ctrlParm, n - k);
}

template <typename ValueType, uint16_t m_order>
ValueType dtBezier<ValueType, m_order>::BinomialCoeff(const int n, const int k) const
{
    if (n < k) return 0;

    int min, max;
    ValueType bc = 1;

    if ((n - k) >= k)
    {
        max = n - k;
        min = k;
    }
    else
    {
        max = k;
        min = n - k;  
    }

    for (int i = 1; i <= min; i++)
    {
        bc *= (ValueType)(max + i) / (ValueType)i;
    }

    return bc;
}

template <typename ValueType, uint16_t m_order>
void dtBezier<ValueType, m_order>::Configure(const ValueType p0, const ValueType pf,
                                             const ValueType v0, const ValueType vf,
                                             const ValueType a0, const ValueType af,
                                             const ValueType *pc, const uint8_t pcSize, const ValueType duration) 
{
  m_num = pcSize + 6;
  m_duration = duration;
  m_p[0] = p0;
  m_p[1] = p0 + v0 / m_num;
  m_p[2] = p0 + 2 * v0 / m_num + a0 / (m_num * (m_num - 1));
  memcpy(&m_p[3], pc, sizeof(ValueType) * pcSize);
  m_p[m_num - 3] = pf - 2 * vf / m_num - af / (m_num * (m_num - 1));
  m_p[m_num - 2] = pf - vf / m_num;
  m_p[m_num - 1] = pf;
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