namespace dtCore {

////////////////////////////////////////////////////////////////////////////////
// Implementation of dtPolynomialTrajectory
//
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtPolynomialTrajectory<ValueType, m_dof, m_order>::dtPolynomialTrajectory()
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
dtPolynomialTrajectory<ValueType, m_dof, m_order>::dtPolynomialTrajectory(const ValueType duration, 
                                                                          const ContRefType pi, const ContRefType pf,
                                                                          const ValueType timeOffset)
: m_duration(duration), m_ti(timeOffset)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 1 || m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);

    ReConfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtPolynomialTrajectory<ValueType, m_dof, m_order>::dtPolynomialTrajectory(const ValueType duration, 
                                                                          const ContRefType pi, const ContRefType pf,
                                                                          const ContRefType vi, const ContRefType vf, 
                                                                          const ValueType timeOffset)
: m_duration(duration), m_ti(timeOffset)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 1 || m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);

    ReConfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtPolynomialTrajectory<ValueType, m_dof, m_order>::dtPolynomialTrajectory(const ValueType duration, 
                                                                          const ContRefType pi, const ContRefType pf,
                                                                          const ContRefType vi, const ContRefType vf, 
                                                                          const ContRefType ai, const ContRefType af, 
                                                                          const ValueType timeOffset)
: m_duration(duration), m_ti(timeOffset)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 1 || m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);

    ReConfigure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtPolynomialTrajectory<ValueType, m_dof, m_order>::~dtPolynomialTrajectory() {}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p) const 
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
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v) const 
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
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const 
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
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::ReConfigure()
{
    for (uint16_t i = 0; i < m_dof; i++) 
    {
        m_interpolator[i].Configure(this->m_pi[i], this->m_pf[i], 
                                    this->m_vi[i], this->m_vf[i], 
                                    this->m_ai[i], this->m_af[i],
                                    this->m_duration);
    }
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, 
                                                                 const ContRefType pi, const ContRefType pf,
                                                                 const ValueType timeOffset) 
{
    m_ti = timeOffset;
    m_duration = duration;

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);

    
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, 
                                                                 const ContRefType pi, const ContRefType pf,
                                                                 const ContRefType vi, const ContRefType vf,
                                                                 const ValueType timeOffset) 
{
    m_ti = timeOffset;
    m_duration = duration;

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, 
                                                                 const ContRefType pi, const ContRefType pf,
                                                                 const ContRefType vi, const ContRefType vf,
                                                                 const ContRefType ai, const ContRefType af,
                                                                 const ValueType timeOffset) 
{
    m_ti = timeOffset;
    m_duration = duration;

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::SetTimeOffset(const ValueType timeOffset) 
{
    m_ti = timeOffset;
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::SetPoint(const ContRefType pi, const ContRefType pf,
                                                                 const ContRefType vi, const ContRefType vf,
                                                                 const ContRefType ai, const ContRefType af) 
{
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtPolynomialTrajectory<ValueType, m_dof, m_order>::SetDuration(const ValueType duration) 
{
    m_duration = duration;
}

} // namespace dtCore