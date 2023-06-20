namespace dtCore {

////////////////////////////////////////////////////////////////////////////////
// Implementation of dtBezierTrajectory
//
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtBezierTrajectory<ValueType, m_dof, m_order>::dtBezierTrajectory(const ValueType duration, 
                                                                  const ContRefType pi, const ContRefType pf,
                                                                  const ContRefType pc, const uint8_t pcSize,
                                                                  const ValueType timeOffset)
: m_duration(duration), m_ti(timeOffset), m_pcSize(pcSize)
{
  static_assert(m_order > 0, "Invalid degree of bezier.");

  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memset(m_vi, 0, sizeof(ValueType) * m_dof);
  memset(m_vf, 0, sizeof(ValueType) * m_dof);
  memset(m_ai, 0, sizeof(ValueType) * m_dof);
  memset(m_af, 0, sizeof(ValueType) * m_dof);

  for (int i = 0; i < m_dof; i++)
  {
      for (int j = 0; j < pcSize; j++)
      {
          m_pc[i][j] = pc[m_dof*j + i]; 
      }
  }

  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtBezierTrajectory<ValueType, m_dof, m_order>::dtBezierTrajectory(const ValueType duration, 
                                                                  const ContRefType pi, const ContRefType pf, 
                                                                  const ContRefType vi, const ContRefType vf, 
                                                                  const ContRefType pc, const uint8_t pcSize,
                                                                  const ValueType timeOffset)
: m_duration(duration), m_ti(timeOffset), m_pcSize(pcSize)
{
  static_assert(m_order > 0, "Invalid degree of bezier.");

  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
  memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
  memset(m_ai, 0, sizeof(ValueType) * m_dof);
  memset(m_af, 0, sizeof(ValueType) * m_dof);
  for (int i = 0; i < m_dof; i++)
  {
      for (int j = 0; j < pcSize; j++)
      {
          m_pc[i][j] = pc[m_dof*j + i]; 
      }
  }

  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtBezierTrajectory<ValueType, m_dof, m_order>::dtBezierTrajectory(const ValueType duration, 
                                                                  const ContRefType pi, const ContRefType pf, 
                                                                  const ContRefType vi, const ContRefType vf, 
                                                                  const ContRefType ai, const ContRefType af, 
                                                                  const ContRefType pc, const uint8_t pcSize,
                                                                  const ValueType timeOffset)
: m_duration(duration), m_ti(timeOffset), m_pcSize(pcSize)
{
  static_assert(m_order > 0, "Invalid degree of bezier.");

  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
  memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
  memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
  memcpy(m_af, af, sizeof(ValueType) * m_dof);
  for (int i = 0; i < m_dof; i++)
  {
      for (int j = 0; j < pcSize; j++)
      {
          m_pc[i][j] = pc[m_dof*j + i]; 
      }
  }

  Reconfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtBezierTrajectory<ValueType, m_dof, m_order>::~dtBezierTrajectory() {}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p) const 
{
    ValueType t_ = t - this->m_ti;
    if (t_ < 0) 
    {
      memcpy(p, this->m_pi, sizeof(ValueType) * m_dof);
    } 
    else if (t_ > this->m_duration) 
    {
      memcpy(p, this->m_pf, sizeof(ValueType) * m_dof);
    } 
    else 
    {
      for (uint16_t i = 0; i < m_dof; i++) 
      {
        m_interpolator[i].Interpolate(t_, p[i]);
      }
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v) const 
{
    ValueType t_ = t - this->m_ti;
    if (t_ < 0) 
    {
      memcpy(p, this->m_pi, sizeof(ValueType) * m_dof);
      memset(v, 0, sizeof(ValueType) * m_dof);
    } 
    else if (t_ > this->m_duration) 
    {
      memcpy(p, this->m_pf, sizeof(ValueType) * m_dof);
      memset(v, 0, sizeof(ValueType) * m_dof);
    } 
    else 
    {
      for (uint16_t i = 0; i < m_dof; i++) 
      {
        m_interpolator[i].Interpolate(t_, p[i], v[i]);
      }
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const 
{
    ValueType t_ = t - this->m_ti;
    if (t_ < 0) 
    {
      memcpy(p, this->m_pi, sizeof(ValueType) * m_dof);
      memset(v, 0, sizeof(ValueType) * m_dof);
      memset(a, 0, sizeof(ValueType) * m_dof);
    } 
    else if (t_ > this->m_duration) 
    {
      memcpy(p, this->m_pf, sizeof(ValueType) * m_dof);
      memset(v, 0, sizeof(ValueType) * m_dof);
      memset(a, 0, sizeof(ValueType) * m_dof);
    } 
    else 
    {
      for (uint16_t i = 0; i < m_dof; i++) 
      {
        m_interpolator[i].Interpolate(t_, p[i], v[i], a[i]);
      }
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::Reconfigure() 
{
    for (uint16_t i = 0; i < m_dof; i++) 
    {
        m_interpolator[i].Configure(this->m_pi[i], this->m_pf[i], this->m_vi[i], this->m_vf[i], this->m_ai[i], this->m_af[i], this->m_pc[i], m_pcSize, this->m_duration);
    }
}


template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, 
                                                             const ContRefType pi, const ContRefType pf, 
                                                             const ContRefType pc, const uint8_t pcSize,
                                                             const ValueType timeOffset)
{
  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memset(m_vi, 0, sizeof(ValueType) * m_dof);
  memset(m_vf, 0, sizeof(ValueType) * m_dof);
  memset(m_ai, 0, sizeof(ValueType) * m_dof);
  memset(m_af, 0, sizeof(ValueType) * m_dof);

  for (int i = 0; i < m_dof; i++)
  {
      for (int j = 0; j < pcSize; j++)
      {
          m_pc[i][j] = pc[m_dof*j + i]; 
      }
  }
  m_pcSize = pcSize;
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, 
                                                             const ContRefType pi, const ContRefType pf, 
                                                             const ContRefType vi, const ContRefType vf, 
                                                             const ContRefType pc, const uint8_t pcSize,
                                                             const ValueType timeOffset)
{
  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
  memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
  memset(m_ai, 0, sizeof(ValueType) * m_dof);
  memset(m_af, 0, sizeof(ValueType) * m_dof);
  for (int i = 0; i < m_dof; i++)
  {
      for (int j = 0; j < pcSize; j++)
      {
          m_pc[i][j] = pc[m_dof*j + i]; 
      }
  }
  m_pcSize = pcSize;  
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, 
                                                             const ContRefType pi, const ContRefType pf, 
                                                             const ContRefType vi, const ContRefType vf, 
                                                             const ContRefType ai, const ContRefType af, 
                                                             const ContRefType pc, const uint8_t pcSize,
                                                             const ValueType timeOffset)
{
  memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
  memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
  memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
  memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
  memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
  memcpy(m_af, af, sizeof(ValueType) * m_dof);
  for (int i = 0; i < m_dof; i++)
  {
      for (int j = 0; j < pcSize; j++)
      {
          m_pc[i][j] = pc[m_dof*j + i]; 
      }
  }
  m_pcSize = pcSize;
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetDuration(const ValueType duration) 
{
  m_duration = duration;
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetInitParam(const ContRefType pi, const ContRefType vi, const ContRefType ai)
{
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    if (vi)
    {
        memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    }
    else
    {
        memset(m_vi, 0, sizeof(ValueType) * m_dof);
    }
    if (ai)
    {
        memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    }
    else
    {
        memset(m_ai, 0, sizeof(ValueType) * m_dof);
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetTargetParam(const ContRefType pf, const ContRefType vf, const ContRefType af)
{
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    if (vf)
    {
        memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    }
    else
    {
        memset(m_vf, 0, sizeof(ValueType) * m_dof);
    }
    if (af)
    {
        memcpy(m_af, af, sizeof(ValueType) * m_dof);
    }
    else
    {
        memset(m_af, 0, sizeof(ValueType) * m_dof);
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetControlParam(const ContRefType pc, const uint8_t pcSize) 
{
    m_pcSize = pcSize;
    memcpy(m_pc, pc, sizeof(ValueType) * m_dof * pcSize);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtBezierTrajectory<ValueType, m_dof, m_order>::SetTimeOffset(const ValueType timeOffset) 
{
  m_ti = timeOffset;
}

} // namespace dtCore