namespace dtCore {

////////////////////////////////////////////////////////////////////////////////
// Implementation of dtScurveTrajectory
//
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory()
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 1 || m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memset(m_pi, 0, sizeof(ValueType) * m_dof);
    memset(m_pf, 0, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                                                                          const ContRefType pi, const ContRefType pf,
                                                                          const ValueType timeOffset)
: m_duration(duration), m_accDuration(accDuration), m_decDuration(accDuration), m_ti(timeOffset), m_isLimitSet(false)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);

    ReConfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                                                                          const ContRefType pi, const ContRefType pf,
                                                                          const ContRefType vi, const ContRefType vf, 
                                                                          const ValueType timeOffset)
: m_duration(duration), m_accDuration(accDuration), m_decDuration(accDuration), m_ti(timeOffset), m_isLimitSet(false)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);

    ReConfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                                                                          const ContRefType pi, const ContRefType pf,
                                                                          const ContRefType vi, const ContRefType vf, 
                                                                          const ContRefType ai, const ContRefType af, 
                                                                          const ValueType timeOffset)
: m_duration(duration), m_accDuration(accDuration), m_decDuration(accDuration), m_ti(timeOffset), m_isLimitSet(false)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);

    ReConfigure();
}

  
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ContRefType vl, const ContRefType al,
                                                                  const ContRefType pi, const ContRefType pf,
                                                                  const ValueType timeOffset)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_vl, vl, sizeof(ValueType) * m_dof);
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    case 5:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 5.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 15.0 / 8.0;
        }
    } break;
    
    default:
    {

    } break;
    }

    ReConfigure();
}
  
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ContRefType vl, const ContRefType al,
                                                                  const ContRefType pi, const ContRefType pf, 
                                                                  const ContRefType vi, const ContRefType vf, 
                                                                  const ValueType timeOffset)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");
    
    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_vl, vl, sizeof(ValueType) * m_dof);
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    case 5:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 5.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 15.0 / 8.0;
        }
    } break;
    
    default:
    {

    } break;
    }

    ReConfigure();
}
  
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ContRefType vl, const ContRefType al,
                                                                  const ContRefType pi, const ContRefType pf, 
                                                                  const ContRefType vi, const ContRefType vf, 
                                                                  const ContRefType ai, const ContRefType af, 
                                                                  const ValueType timeOffset)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_vl, vl, sizeof(ValueType) * m_dof);
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    case 5:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 5.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 15.0 / 8.0;
        }
    } break;
    
    default:
    {

    } break;
    }

    ReConfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::~dtScurveTrajectory() {}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p) const 
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
void dtScurveTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v) const 
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
void dtScurveTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const 
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
void dtScurveTrajectory<ValueType, m_dof, m_order>::ReConfigure()
{
    if (m_isLimitSet)
    {
        CalculateMaxVelocity();
        CalculateDuration();
    }

    for (uint16_t i = 0; i < m_dof; i++) 
    {
        m_vm[i] = (m_pf[i] - m_pi[i] - 0.5 * m_vi[i] * m_accDuration - 0.5 * m_vf[i] * m_decDuration) / (m_duration - 0.5 * (m_accDuration + m_decDuration));
        m_interpolator[i].Configure(this->m_pi[i], this->m_pf[i], 
                                    this->m_vi[i], this->m_vf[i], 
                                    this->m_ai[i], this->m_af[i],
                                    this->m_vm[i], this->m_duration,
                                    this->m_accDuration, this->m_decDuration);
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, const ValueType accDuration, 
                                                                 const ContRefType pi, const ContRefType pf,
                                                                 const ValueType timeOffset) 
{
    m_ti = timeOffset;
    m_duration = duration;
    m_accDuration = accDuration;
    m_decDuration = accDuration;
    m_isLimitSet = false;
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, const ValueType accDuration, 
                                                                 const ContRefType pi, const ContRefType pf,
                                                                 const ContRefType vi, const ContRefType vf,
                                                                 const ValueType timeOffset) 
{
    m_ti = timeOffset;
    m_duration = duration;
    m_accDuration = accDuration;
    m_decDuration = accDuration;
    m_isLimitSet = false;
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, const ValueType accDuration, 
                                                                 const ContRefType pi, const ContRefType pf,
                                                                 const ContRefType vi, const ContRefType vf,
                                                                 const ContRefType ai, const ContRefType af,
                                                                 const ValueType timeOffset) 
{
    m_ti = timeOffset;
    m_duration = duration;
    m_accDuration = accDuration;
    m_decDuration = accDuration;
    m_isLimitSet = false;
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ContRefType vl, const ContRefType al,
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ValueType timeOffset)
{
    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_vl, vl, sizeof(ValueType) * m_dof);
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    case 5:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 5.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 15.0 / 8.0;
        }
    } break;
    
    default:
    {

    } break;
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ContRefType vl, const ContRefType al,
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ContRefType vi, const ContRefType vf,
                                                             const ValueType timeOffset)
{
    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_vl, vl, sizeof(ValueType) * m_dof);
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    case 5:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 5.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 15.0 / 8.0;
        }
    } break;
    
    default:
    {

    } break;
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ContRefType vl, const ContRefType al,
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ContRefType vi, const ContRefType vf,
                                                             const ContRefType ai, const ContRefType af,
                                                             const ValueType timeOffset)
{
    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_vl, vl, sizeof(ValueType) * m_dof);
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    case 5:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 5.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 15.0 / 8.0;
        }
    } break;
    
    default:
    {

    } break;
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetDuration(const ValueType duration, const ValueType accDuration) 
{
    m_isLimitSet = false;
    m_duration = duration;
    m_accDuration = accDuration;
    m_decDuration = accDuration;
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetInitParam(const ContRefType pi, const ContRefType vi, const ContRefType ai) 
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
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetTargetParam(const ContRefType pf, const ContRefType vf, const ContRefType af) 
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
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetTimeOffset(const ValueType timeOffset) 
{
    m_ti = timeOffset;
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetLimit(const ContRefType vl, const ContRefType al)
{
    m_isLimitSet = true;
    switch (m_order)
    {
    case 5:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 5.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (int i = 0; i < m_dof; i++)
        {
            m_al[i] = al[i] * 15.0 / 8.0;
        }
    } break;
    
    default:
    {

    } break;
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::CalculateMaxVelocity()
{
    ValueType minDistance[m_dof] = {0, };
    for (int i = 0; i < m_dof; i++)
    {
        minDistance[i] = 0.5 * (m_vi[i] + m_vf[i]) * std::abs(m_vi[i] - m_vf[i]) / m_al[i];
        if (minDistance[i] <= (m_pf[i] - m_pi[i]))
        {
            ValueType vm = std::sqrt(0.5 * (std::pow(m_vi[i], 2) + std::pow(m_vf[i], 2)) + (m_pf[i] - m_pi[i]) * m_al[i]);
            m_vm[i] = m_vl[i] < vm ? m_vl[i] : vm;
        } 
        else
        {
            ValueType vm = -std::sqrt(0.5 * (std::pow(m_vi[i], 2) + std::pow(m_vf[i], 2)) - (m_pf[i] - m_pi[i]) * m_al[i]);
            m_vm[i] = vm < -m_vl[i] ? -m_vl[i] : vm;
        }
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::CalculateDuration()
{
    m_accDuration = 0;
    m_decDuration = 0;
    m_conDuration = 0;

    for (int i = 0; i < m_dof; i++)
    {
        ValueType accDuration = std::abs(m_vm[i] - m_vi[i]) / m_al[i];
        ValueType decDuration = std::abs(m_vf[i] - m_vm[i]) / m_al[i];
        m_accDuration = m_accDuration < accDuration ? accDuration : m_accDuration;
        m_decDuration = m_decDuration < decDuration ? decDuration : m_decDuration;

        ValueType conDuration = ((m_pf[i] - m_pi[i]) - 0.5 * (m_vi[i] + m_vm[i]) * m_accDuration - 0.5 * (m_vm[i] + m_vf[i]) * m_decDuration) / m_vm[i];
        m_conDuration = m_conDuration < conDuration ? conDuration : m_conDuration;
    }
    
    m_duration = m_accDuration + m_conDuration + m_decDuration;
}

} // namespace dtCore