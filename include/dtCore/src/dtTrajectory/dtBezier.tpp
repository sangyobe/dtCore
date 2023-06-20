namespace dtCore {

template <typename ValueType, uint16_t m_maxNum>
dtBezier<ValueType, m_maxNum>::dtBezier() 
{
    static_assert(m_maxNum >= 1, "Invalid degree of bezier curve.");
}

template <typename ValueType, uint16_t m_maxNum>
dtBezier<ValueType, m_maxNum>::~dtBezier() {}

/*! \details Calculates the desired position(p) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
*/
template <typename ValueType, uint16_t m_maxNum>
void dtBezier<ValueType, m_maxNum>::Interpolate(const ValueType t, ValueType &p) const 
{
    ValueType pos = 0;
    const ValueType tc = t / m_duration;
    
    for (int i = 0; i < m_num; i++)
    {
        pos += m_posCoeff[i] * pow(tc, i) * pow(1 - tc, m_num - 1 - i) * m_p[i];
    }

    p = pos;
}

/*! \details Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
    \param[out] v desired velocity (x/sec)
*/
template <typename ValueType, uint16_t m_maxNum>
void dtBezier<ValueType, m_maxNum>::Interpolate(const ValueType t, ValueType &p, ValueType &v) const 
{
    ValueType pos = 0;
    ValueType vel = 0;
    const ValueType tc = t / m_duration;
    
    for (int i = 0; i < m_num - 1; i++)
    {
        pos += m_posCoeff[i] * pow(tc, i) * pow(1 - tc, m_num - 1 - i) * m_p[i];
        vel += m_velCoeff[i] * pow(tc, i) * pow(1 - tc, m_num - 2 - i) * (m_p[i + 1] - m_p[i]) * (m_num - 1) ;
    }
    pos += m_posCoeff[m_num - 1] * pow(tc, m_num - 1) * pow(1 - tc, 0) * m_p[m_num - 1];

    p = pos;
    v = vel;
}

/*! \details Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
    \param[out] v desired velocity (x/sec)
    \param[out] a desired acceleration (x/sec^2)
*/
template <typename ValueType, uint16_t m_maxNum>
void dtBezier<ValueType, m_maxNum>::Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const 
{
    ValueType pos = 0;
    ValueType vel = 0;
    ValueType acc = 0;
    const ValueType tc = t / m_duration;
    
    for (int i = 0; i < m_num - 2; i++)
    {
        pos += m_posCoeff[i] * pow(tc, i) * pow(1 - tc, m_num - 1 - i) * m_p[i];
        vel += m_velCoeff[i] * pow(tc, i) * pow(1 - tc, m_num - 2 - i) * (m_p[i + 1] - m_p[i]) * (m_num - 1);
        acc += m_accCoeff[i] * pow(tc, i) * pow(1 - tc, m_num - 3 - i) * ((m_p[i + 2] - m_p[i + 1]) - (m_p[i + 1] - m_p[i])) * (m_num - 1) * (m_num - 2) ;
    }
    pos += m_posCoeff[m_num - 2] * pow(tc, m_num - 2) * pow(1 - tc, 1) * m_p[m_num - 2];
    pos += m_posCoeff[m_num - 1] * pow(tc, m_num - 1) * pow(1 - tc, 0) * m_p[m_num - 1];
    vel += m_velCoeff[m_num - 2] * pow(tc, m_num - 2) * pow(1 - tc, 0) * (m_p[m_num - 1] - m_p[m_num - 2]) * (m_num - 1);

    p = pos;
    v = vel;
    a = acc;
}

/*! \details Configure the control points and coefficients of the bezier trajectory from the parameters entered.
    \param[in] p0 init position (x)
    \param[in] pf target position (x)
    \param[in] v0 init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] a0 init acceleration (x/sec^2)
    \param[in] af target acceleration (x/sec^2)
    \param[in] pc control point (x)
    \param[in] pcNum pc size
    \param[in] duration polynomial trajectory duration (sec)
*/
template <typename ValueType, uint16_t m_maxNum>
void dtBezier<ValueType, m_maxNum>::Configure(const ValueType p0, const ValueType pf,
                                             const ValueType v0, const ValueType vf,
                                             const ValueType a0, const ValueType af,
                                             const ValueType *pc, const uint8_t pcNum, const ValueType duration) 
{
    m_num = pcNum + 6;
    m_duration = duration;
    m_p[0] = p0;
    m_p[1] = p0 + v0 / (m_num - 1);
    m_p[2] = p0 + 2 * v0 / (m_num - 1) + a0 / ((m_num - 1) * (m_num - 2));
    memcpy(&m_p[3], pc, sizeof(ValueType) * pcNum);
    m_p[m_num - 3] = pf - 2 * vf / (m_num - 1) - af / ((m_num - 1) * (m_num - 2));
    m_p[m_num - 2] = pf - vf / (m_num - 1);
    m_p[m_num - 1] = pf;

    for (int i = 0; i < m_num - 2; i++)
    {
        m_posCoeff[i] = BinomialCoeff(m_num - 1, i);
        m_velCoeff[i] = BinomialCoeff(m_num - 2, i);
        m_accCoeff[i] = BinomialCoeff(m_num - 3, i);
    }
    m_posCoeff[m_num - 2] = BinomialCoeff(m_num - 1, m_num - 2);
    m_velCoeff[m_num - 2] = BinomialCoeff(m_num - 2, m_num - 2);
    m_posCoeff[m_num - 1] = BinomialCoeff(m_num - 1, m_num - 1);
}

/*! \details Calculate binomial coefficients.
    \param[in] n n'th binomial
    \param[in] k kth out of n
*/
template <typename ValueType, uint16_t m_maxNum>
ValueType dtBezier<ValueType, m_maxNum>::BinomialCoeff(const int n, const int k) const
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
} // namespace dtCore