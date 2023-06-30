namespace dtCore {

////////////////////////////////////////////////////////////////////////////////
// Implementation of dtScurveTrajectory
//

/*! \details Initialize without input parameter
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory()
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memset(m_pi, 0, sizeof(ValueType) * m_dof);
    memset(m_pf, 0, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
}

/*! \details Initialize and configure the coefficients of the polynomial from the parameters entered.
    \param[in] duration trajectory duration (sec)
    \param[in] accDuration trajectory acc/dec duration (sec)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [vi(init velocity), vf(target velocity), ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                                                                  const ContRefType pi, const ContRefType pf,
                                                                  const ValueType timeOffset)
: m_duration(duration), m_accDuration(accDuration), m_decDuration(accDuration), m_ti(timeOffset), m_isLimitSet(false)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");
    assert(duration > m_tolerance && "Trajectory duration should be greater than zero");
    assert(accDuration > m_tolerance && "Trajectory acceleration duration should be greater than zero");
    assert(duration - 2 * accDuration >= m_tolerance && "Trajectory acceleration duration should be smaller than half of duration");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);

    Configure();
}

/*! \details Initialize and configure the coefficients of the polynomial from the parameters entered.
    \param[in] duration trajectory duration (sec)
    \param[in] accDuration trajectory acc/dec duration (sec)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ValueType duration, const ValueType accDuration,
                                                                  const ContRefType pi, const ContRefType pf,
                                                                  const ContRefType vi, const ContRefType vf, 
                                                                  const ValueType timeOffset)
: m_duration(duration), m_accDuration(accDuration), m_decDuration(accDuration), m_ti(timeOffset), m_isLimitSet(false)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");
    assert(duration > m_tolerance && "Trajectory duration should be greater than zero");
    assert(accDuration > m_tolerance && "Trajectory acceleration duration should be greater than zero");
    assert(duration - 2 * accDuration >= m_tolerance && "Trajectory acceleration duration should be smaller than half of duration");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);

    Configure();
}

/*! \details Initialize and configure the coefficients of the polynomial from the parameters entered.
    \param[in] duration trajectory duration (sec)
    \param[in] accDuration trajectory acc/dec duration (sec)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] ai init acceleration (x/sec^2)
    \param[in] af target acceleration (x/sec^2)
    \param[in] timeOffset trajectory offset(delay) (sec)
*/
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
    assert(duration > m_tolerance && "Trajectory duration should be greater than zero");
    assert(accDuration > m_tolerance && "Trajectory acceleration duration should be greater than zero");
    assert(duration - 2 * accDuration >= m_tolerance && "Trajectory acceleration duration should be smaller than half of duration");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);

    Configure();
}

/*! \details Initialize and configure the coefficients of the polynomial from the parameters entered.
    \param[in] vLimit trajectory limit velocity (x/sec)
    \param[in] aLimit trajectory limit acceleration (x/sec^2)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [vi(init velocity), vf(target velocity), ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ContRefType vLimit, const ContRefType aLimit,
                                                                  const ContRefType pi, const ContRefType pf,
                                                                  const ValueType timeOffset)
: m_ti(timeOffset), m_isLimitSet(true)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    // Velocity, acceleration limits can be entered as positive or negative.
    // Calculate average acceleration limit
    case 5:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 2.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 8.0 / 15.0;
        }
    } break;
    
    default:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]);
        }
    } break;
    }

    Configure();
}

/*! \details Initialize and configure the coefficients of the polynomial from the parameters entered.
    \param[in] vLimit trajectory limit velocity (x/sec)
    \param[in] aLimit trajectory limit acceleration (x/sec^2)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ContRefType vLimit, const ContRefType aLimit,
                                                                  const ContRefType pi, const ContRefType pf, 
                                                                  const ContRefType vi, const ContRefType vf, 
                                                                  const ValueType timeOffset)
: m_ti(timeOffset), m_isLimitSet(true)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");
    
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    // Velocity, acceleration limits can be entered as positive or negative.
    // Calculate average acceleration limit
    case 5:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 2.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 8.0 / 15.0;
        }
    } break;
    
    default:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]);
        }
    } break;
    }

    Configure();
}

/*! \details Initialize and configure the coefficients of the polynomial from the parameters entered.
    \param[in] vLimit trajectory limit velocity (x/sec)
    \param[in] aLimit trajectory limit acceleration (x/sec^2)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] ai init acceleration (x/sec^2)
    \param[in] af target acceleration (x/sec^2)
    \param[in] timeOffset trajectory offset(delay) (sec)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::dtScurveTrajectory(const ContRefType vLimit, const ContRefType aLimit,
                                                                  const ContRefType pi, const ContRefType pf, 
                                                                  const ContRefType vi, const ContRefType vf, 
                                                                  const ContRefType ai, const ContRefType af, 
                                                                  const ValueType timeOffset)
: m_ti(timeOffset), m_isLimitSet(true)
{
    static_assert(m_dof > 0, "Trajectory dimension(m_dof) should be greater than zero.");
    static_assert(m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");

    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    // Velocity, acceleration limits can be entered as positive or negative.
    // Calculate average acceleration limit
    case 5:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 2.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 8.0 / 15.0;
        }
    } break;
    
    default:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]);
        }
    } break;
    }

    Configure();
}

template <typename ValueType, uint16_t m_dof, uint16_t m_order>
dtScurveTrajectory<ValueType, m_dof, m_order>::~dtScurveTrajectory() {}

/*! \details Calculates the desired position(p) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p) const 
{
    ValueType t_ = t - this->m_ti;

    if (t_ < 0) 
    {
        // before trajectory start
        memcpy(p, this->m_pi, sizeof(ValueType) * m_dof);
    } 
    else if (t_ > this->m_duration) 
    {
        // after trajectory end
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

/*! \details Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
    \param[out] v desired velocity (x/sec)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v) const 
{
    ValueType t_ = t - this->m_ti;

    if (t_ < 0) 
    {
        // before trajectory start
        memcpy(p, this->m_pi, sizeof(ValueType) * m_dof);
        memset(v, 0, sizeof(ValueType) * m_dof);
    } 
    else if (t_ > this->m_duration) 
    {
        // after trajectory end
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

/*! \details Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
    \param[out] v desired velocity (x/sec)
    \param[out] a desired acceleration (x/sec^2)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::Interpolate(const ValueType t, ContRefType p, ContRefType v, ContRefType a) const 
{
    ValueType t_ = t - this->m_ti;

    if (t_ < 0) 
    {
        // before trajectory start
        memcpy(p, this->m_pi, sizeof(ValueType) * m_dof);
        memset(v, 0, sizeof(ValueType) * m_dof);
        memset(a, 0, sizeof(ValueType) * m_dof);
    } 
    else if (t_ > this->m_duration) 
    {
        // after trajectory end
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

/*! \details Configure the coefficients of s-curve through parameters entered from functions below 
             (SetParam, SetDuration, SetInitParam, SetTargetParam, SetTimeOffset, SetLimit).
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::Configure()
{
    if (m_isLimitSet) //!< The last parameter you entered is a parameter related to velocity, acceleration limits
    {
        CalculateLinearVelocity();
        CalculateDuration();
    }

    for (uint16_t i = 0; i < m_dof; i++) 
    {
        // Calculate new velocity limits
        m_vLimit[i] = (m_pf[i] - m_pi[i] - 0.5 * m_vi[i] * m_accDuration - 0.5 * m_vf[i] * m_decDuration) / (m_duration - 0.5 * (m_accDuration + m_decDuration));
        m_interpolator[i].Configure(this->m_pi[i], this->m_pf[i], 
                                    this->m_vi[i], this->m_vf[i], 
                                    this->m_ai[i], this->m_af[i],
                                    this->m_vLimit[i], this->m_duration,
                                    this->m_accDuration, this->m_decDuration);
    }
}

/*! \details Enter parameters for the Configure() function.
    \param[in] duration trajectory duration (sec)
    \param[in] accDuration trajectory acc/dec duration (sec)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [vi(init velocity), vf(target velocity), ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, const ValueType accDuration, 
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ValueType timeOffset) 
{
    assert(duration > m_tolerance && "Trajectory duration should be greater than zero");
    assert(accDuration > m_tolerance && "Trajectory acceleration duration should be greater than zero");
    assert(duration - 2 * accDuration >= m_tolerance && "Trajectory acceleration duration should be smaller than half of duration");

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

/*! \details Enter parameters for the Configure() function.
    \param[in] duration trajectory duration (sec)
    \param[in] accDuration trajectory acc/dec duration (sec)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, const ValueType accDuration, 
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ContRefType vi, const ContRefType vf,
                                                             const ValueType timeOffset) 
{
    assert(duration > m_tolerance && "Trajectory duration should be greater than zero");
    assert(accDuration > m_tolerance && "Trajectory acceleration duration should be greater than zero");
    assert(duration - 2 * accDuration >= m_tolerance && "Trajectory acceleration duration should be smaller than half of duration");

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

/*! \details Enter parameters for the Configure() function.
    \param[in] duration trajectory duration (sec)
    \param[in] accDuration trajectory acc/dec duration (sec)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] ai init acceleration (x/sec^2)
    \param[in] af target acceleration (x/sec^2)
    \param[in] timeOffset trajectory offset(delay) (sec)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ValueType duration, const ValueType accDuration, 
                                                                 const ContRefType pi, const ContRefType pf,
                                                                 const ContRefType vi, const ContRefType vf,
                                                                 const ContRefType ai, const ContRefType af,
                                                                 const ValueType timeOffset) 
{
    assert(duration > m_tolerance && "Trajectory duration should be greater than zero");
    assert(accDuration > m_tolerance && "Trajectory acceleration duration should be greater than zero");
    assert(duration - 2 * accDuration >= m_tolerance && "Trajectory acceleration duration should be smaller than half of duration");

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

/*! \details Enter parameters for the Configure() function.
    \param[in] vLimit trajectory limit velocity (x/sec)
    \param[in] aLimit trajectory limit acceleration (x/sec^2)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [vi(init velocity), vf(target velocity), ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ContRefType vLimit, const ContRefType aLimit,
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ValueType timeOffset)
{
    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memset(m_vi, 0, sizeof(ValueType) * m_dof);
    memset(m_vf, 0, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    // Velocity, acceleration limits can be entered as positive or negative.
    // Calculate average acceleration limit
    case 5:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 2.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 8.0 / 15.0;
        }
    } break;
    
    default:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]);
        }
    } break;
    }
}

/*! \details Enter parameters for the Configure() function.
    \param[in] vLimit trajectory limit velocity (x/sec)
    \param[in] aLimit trajectory limit acceleration (x/sec^2)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] timeOffset trajectory offset(delay) (sec)
    The parameters [ai(init acceleration), af(target acceleration)] that are not entered are set to zero.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ContRefType vLimit, const ContRefType aLimit,
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ContRefType vi, const ContRefType vf,
                                                             const ValueType timeOffset)
{
    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memset(m_ai, 0, sizeof(ValueType) * m_dof);
    memset(m_af, 0, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    // Velocity, acceleration limits can be entered as positive or negative.
    // Calculate average acceleration limit
    case 5:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 2.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 8.0 / 15.0;
        }
    } break;
    
    default:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]);
        }
    } break;
    }
}

/*! \details Enter parameters for the Configure() function.
    \param[in] vLimit trajectory limit velocity (x/sec)
    \param[in] aLimit trajectory limit acceleration (x/sec^2)
    \param[in] pi init position (x)
    \param[in] pf target position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] ai init acceleration (x/sec^2)
    \param[in] af target acceleration (x/sec^2)
    \param[in] timeOffset trajectory offset(delay) (sec)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetParam(const ContRefType vLimit, const ContRefType aLimit,
                                                             const ContRefType pi, const ContRefType pf,
                                                             const ContRefType vi, const ContRefType vf,
                                                             const ContRefType ai, const ContRefType af,
                                                             const ValueType timeOffset)
{
    m_ti = timeOffset;
    m_isLimitSet = true;
    memcpy(m_pi, pi, sizeof(ValueType) * m_dof);
    memcpy(m_pf, pf, sizeof(ValueType) * m_dof);
    memcpy(m_vi, vi, sizeof(ValueType) * m_dof);
    memcpy(m_vf, vf, sizeof(ValueType) * m_dof);
    memcpy(m_ai, ai, sizeof(ValueType) * m_dof);
    memcpy(m_af, af, sizeof(ValueType) * m_dof);
    switch (m_order)
    {
    // Velocity, acceleration limits can be entered as positive or negative.
    // Calculate average acceleration limit
    case 5:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 2.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 8.0 / 15.0;
        }
    } break;
    
    default:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]);
        }
    } break;
    }
}

/*! \details  Enter trajectory duration for the Configure() function.
    \param[in] duration trajectory duration (sec)
    \param[in] accDuration trajectory acc/dec duration (sec)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetDuration(const ValueType duration, const ValueType accDuration) 
{
    assert(duration > m_tolerance && "Trajectory duration should be greater than zero");
    assert(accDuration > m_tolerance && "Trajectory acceleration duration should be greater than zero");
    assert(duration - 2 * accDuration >= m_tolerance && "Trajectory acceleration duration should be smaller than half of duration");

    m_isLimitSet = false;
    m_duration = duration;
    m_accDuration = accDuration;
    m_decDuration = accDuration;
}

/*! \details Enter init parameter for the Configure() function.
    \param[in] pi init position (x)
    \param[in] vi init velocity (x/sec)
    \param[in] ai init acceleration (x/sec^2)
    The parameters [vi(init velocity), ai(init acceleration)] that are not entered are set to zero.
*/
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

/*! \details Enter target parameter for the Configure() function.
    \param[in] pf target position (x)
    \param[in] vf target velocity (x/sec)
    \param[in] af target acceleration (x/sec^2)
    The parameters [vf(target velocity), af(target acceleration)] that are not entered are set to zero.
*/
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

/*! \details Enter trajectory delay for the Configure() function.
    \param[in] timeOffset trajectory delay (sec)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetTimeOffset(const ValueType timeOffset) 
{
    m_ti = timeOffset;
}

/*! \details Enter trajectory limit for the Configure() function.
    \param[in] vLimit trajectory limit velocity (x/sec)
    \param[in] aLimit trajectory limit acceleration (x/sec^2)
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::SetLimit(const ContRefType vLimit, const ContRefType aLimit)
{
    m_isLimitSet = true;
    switch (m_order)
    {
    // Velocity, acceleration limits can be entered as positive or negative.
    // Calculate average acceleration limit
    case 5:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 2.0 / 3.0;
        }
    } break;

    case 7:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]) * 8.0 / 15.0;
        }
    } break;
    
    default:
    {
        for (uint16_t i = 0; i < m_dof; i++)
        {
            m_vLimit[i] = abs(vLimit[i]);
            m_aLimit[i] = abs(aLimit[i]);
        }
    } break;
    }
}

/*! \details Calculate linear velocity.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::CalculateLinearVelocity()
{
    ValueType minDistance[m_dof] = {0, };
    for (uint16_t i = 0; i < m_dof; i++)
    {   
        // Calculate minimum distance required to reach the target velocity from the init velocity while maintaining the limite acceleration
        minDistance[i] = 0.5 * (m_vi[i] + m_vf[i]) * abs(m_vi[i] - m_vf[i]) / m_aLimit[i];
        // Compare the limit velocity with the maximum velocity that meets the acceleration limit and distance
        if (minDistance[i] <= (m_pf[i] - m_pi[i]))
        {
            ValueType vm = sqrt(0.5 * (pow(m_vi[i], 2) + pow(m_vf[i], 2)) + (m_pf[i] - m_pi[i]) * m_aLimit[i]);
            m_vLimit[i] = m_vLimit[i] < vm ? m_vLimit[i] : vm;
        } 
        else
        {
            ValueType vm = -sqrt(0.5 * (pow(m_vi[i], 2) + pow(m_vf[i], 2)) - (m_pf[i] - m_pi[i]) * m_aLimit[i]);
            m_vLimit[i] = vm < -m_vLimit[i] ? -m_vLimit[i] : vm;
        }
    }
}

/*! \details Calculate the longest acceleration, constant velocity, and deceleration durations for the entire degree of freedom.
*/
template <typename ValueType, uint16_t m_dof, uint16_t m_order>
void dtScurveTrajectory<ValueType, m_dof, m_order>::CalculateDuration()
{
    m_accDuration = 0;
    m_decDuration = 0;
    m_conDuration = 0;

    for (uint16_t i = 0; i < m_dof; i++)
    {
        ValueType accDuration = abs(m_vLimit[i] - m_vi[i]) / m_aLimit[i];
        ValueType decDuration = abs(m_vf[i] - m_vLimit[i]) / m_aLimit[i];
        m_accDuration = m_accDuration < accDuration ? accDuration : m_accDuration;
        m_decDuration = m_decDuration < decDuration ? decDuration : m_decDuration;

        ValueType conDuration = ((m_pf[i] - m_pi[i]) - 0.5 * (m_vi[i] + m_vLimit[i]) * m_accDuration - 0.5 * (m_vLimit[i] + m_vf[i]) * m_decDuration) / m_vLimit[i];
        m_conDuration = m_conDuration < conDuration ? conDuration : m_conDuration;
    }
    
    m_duration = m_accDuration + m_conDuration + m_decDuration;
}

} // namespace dtCore