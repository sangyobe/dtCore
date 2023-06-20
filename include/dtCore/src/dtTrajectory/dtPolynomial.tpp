namespace dtCore {

template <typename ValueType, uint16_t m_order>
dtPolynomial<ValueType, m_order>::dtPolynomial() 
{
    static_assert(m_order == 1 || m_order == 3 || m_order == 5 || m_order == 7, "Invalid degree of polynomial.");
}

template <typename ValueType, uint16_t m_order>
dtPolynomial<ValueType, m_order>::~dtPolynomial() {}

/*! \details Calculates the desired position(p) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
*/
template <typename ValueType, uint16_t m_order>
void dtPolynomial<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p) const 
{
    switch (m_order) {
    case 1: 
    {
        p = m_coeff[0] + m_coeff[1] * t;
    } break;

    case 3: 
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        p =     m_coeff[0]      + m_coeff[1] * t
          +     m_coeff[2] * t2 + m_coeff[3] * t3;
    } break;

    case 5: 
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        p =      m_coeff[0]      +      m_coeff[1] * t 
          +      m_coeff[2] * t2 +      m_coeff[3] * t3
          +      m_coeff[4] * t4 +      m_coeff[5] * t5;
    } break;

    case 7:
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        const ValueType t6 = t * t5;
        const ValueType t7 = t * t6;
        p =      m_coeff[0]      +      m_coeff[1] * t 
          +      m_coeff[2] * t2 +      m_coeff[3] * t3
          +      m_coeff[4] * t4 +      m_coeff[5] * t5
          +      m_coeff[6] * t6 +      m_coeff[7] * t7;
    } break;

    default:
        assert(false && "Invalid degree of polynomial.");
    break;
    }
}

/*! \details Calculates the desired position(p) and velocity(v) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
    \param[out] v desired velocity (x/sec)
*/
template <typename ValueType, uint16_t m_order>
void dtPolynomial<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p, ValueType &v) const 
{
    switch (m_order) {
    case 1: 
    {
        p = m_coeff[0] + m_coeff[1] * t;
        v = m_coeff[1];
    } break;

    case 3: 
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        p =     m_coeff[0]      + m_coeff[1] * t
          +     m_coeff[2] * t2 + m_coeff[3] * t3;
        v =     m_coeff[1] + 2 * m_coeff[2] * t 
          + 3 * m_coeff[3] * t2;
    } break;

    case 5: 
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        p =      m_coeff[0]      +      m_coeff[1] * t 
          +      m_coeff[2] * t2 +      m_coeff[3] * t3
          +      m_coeff[4] * t4 +      m_coeff[5] * t5;
        v =      m_coeff[1]      +  2 * m_coeff[2] * t
          +  3 * m_coeff[3] * t2 +  4 * m_coeff[4] * t3 
          +  5 * m_coeff[5] * t4;
    } break;

    case 7:
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        const ValueType t6 = t * t5;
        const ValueType t7 = t * t6;
        p =      m_coeff[0]      +      m_coeff[1] * t 
          +      m_coeff[2] * t2 +      m_coeff[3] * t3
          +      m_coeff[4] * t4 +      m_coeff[5] * t5
          +      m_coeff[6] * t6 +      m_coeff[7] * t7;
        v =      m_coeff[1]      +  2 * m_coeff[2] * t
          +  3 * m_coeff[3] * t2 +  4 * m_coeff[4] * t3 
          +  5 * m_coeff[5] * t4 +  4 * m_coeff[6] * t5 
          +  7 * m_coeff[7] * t6;
    } break;

    default:
        assert(false && "Invalid degree of polynomial.");
    break;
    }
}

/*! \details Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 
    \param[in] t current time (sec)
    \param[out] p desired position (x)
    \param[out] v desired velocity (x/sec)
    \param[out] a desired acceleration (x/sec^2)
*/
template <typename ValueType, uint16_t m_order>
void dtPolynomial<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const 
{
    switch (m_order) {
    case 1: 
    {
        p = m_coeff[0] + m_coeff[1] * t;
        v = m_coeff[1];
        a = 0.0;
    } break;

    case 3: 
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        p =     m_coeff[0]      + m_coeff[1] * t
          +     m_coeff[2] * t2 + m_coeff[3] * t3;
        v =     m_coeff[1] + 2 * m_coeff[2] * t 
          + 3 * m_coeff[3] * t2;
        a = 2 * m_coeff[2] + 3 * m_coeff[3] * t;
    } break;

    case 5: 
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        p =      m_coeff[0]      +      m_coeff[1] * t 
          +      m_coeff[2] * t2 +      m_coeff[3] * t3
          +      m_coeff[4] * t4 +      m_coeff[5] * t5;
        v =      m_coeff[1]      +  2 * m_coeff[2] * t
          +  3 * m_coeff[3] * t2 +  4 * m_coeff[4] * t3 
          +  5 * m_coeff[5] * t4;
        a =  2 * m_coeff[2]      +  6 * m_coeff[3] * t 
          + 12 * m_coeff[4] * t2 + 20 * m_coeff[5] * t3;
    } break;

    case 7:
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        const ValueType t6 = t * t5;
        const ValueType t7 = t * t6;
        p =      m_coeff[0]      +      m_coeff[1] * t 
          +      m_coeff[2] * t2 +      m_coeff[3] * t3
          +      m_coeff[4] * t4 +      m_coeff[5] * t5
          +      m_coeff[6] * t6 +      m_coeff[7] * t7;
        v =      m_coeff[1]      +  2 * m_coeff[2] * t
          +  3 * m_coeff[3] * t2 +  4 * m_coeff[4] * t3 
          +  5 * m_coeff[5] * t4 +  6 * m_coeff[6] * t5 
          +  7 * m_coeff[7] * t6;
        a =  2 * m_coeff[2]      +  6 * m_coeff[3] * t 
          + 12 * m_coeff[4] * t2 + 20 * m_coeff[5] * t3
          + 30 * m_coeff[6] * t4 + 42 * m_coeff[7] * t5;
    } break;

    default:
        assert(false && "Invalid degree of polynomial.");
    break;
    }
}

/*! \details Calculates the desired position(p), velocity(v) and acceleration(a) corresponding to the time(t) entered. 
    \param[in] p0 init position (x)
    \param[in] pf target position (x)
    \param[in] v0 init velocity (x/sec)
    \param[in] vf target velocity (x/sec)
    \param[in] a0 init acceleration (x/sec^2)
    \param[in] af target acceleration (x/sec^2)
    \param[in] duration polynomial trajectory duration (sec)
*/
template <typename ValueType, uint16_t m_order>
void dtPolynomial<ValueType, m_order>::Configure(const ValueType p0, const ValueType pf, 
                                                 const ValueType v0, const ValueType vf, 
                                                 const ValueType a0, const ValueType af,
                                                 const ValueType duration) 
{
    const ValueType t = duration;
    switch (m_order) {
    case 1:
    {
        m_coeff[0] = p0;
        m_coeff[1] = (pf - p0) / t;
    } break;
    case 3:
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        m_coeff[0] = p0;
        m_coeff[1] = v0;
        m_coeff[2] = (( 3 * pf - 3 * p0) - (vf + 2 * v0) * t) / t2;
        m_coeff[3] = ((-2 * pf + 2 * p0) + (vf +     v0) * t) / t3;
    } break;
    case 5:
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        m_coeff[0] = p0;
        m_coeff[1] = v0;
        m_coeff[2] = 0.5 * a0;
        m_coeff[3] = (( 10 * pf - 10 * p0) - (4 * vf + 6 * v0) * t + (0.5 * af - 1.5 * a0) * t2) / t3;
        m_coeff[4] = ((-15 * pf + 15 * p0) + (7 * vf + 8 * v0) * t - (      af - 1.5 * a0) * t2) / t4;
        m_coeff[5] = ((  6 * pf -  6 * p0) - (3 * vf + 3 * v0) * t + (0.5 * af - 0.5 * a0) * t2) / t5;
    } break;
    case 7:
    {
        const ValueType t2 = t * t;
        const ValueType t3 = t * t2;
        const ValueType t4 = t * t3;
        const ValueType t5 = t * t4;
        const ValueType t6 = t * t5;
        const ValueType t7 = t * t6;
        m_coeff[0] = p0;
        m_coeff[1] = v0;
        m_coeff[2] = 0.5 * a0;
        m_coeff[3] = 0;
        m_coeff[4] = (( 35 * pf - 35 * p0) - (15 * vf + 20 * v0) * t + (2.5 * af - 5.0 * a0) * t2) / t4;
        m_coeff[5] = ((-84 * pf + 84 * p0) + (39 * vf + 45 * v0) * t - (  7 * af -  10 * a0) * t2) / t5;
        m_coeff[6] = ( (70 * pf - 70 * p0) - (34 * vf + 36 * v0) * t + (6.5 * af - 7.5 * a0) * t2) / t6;
        m_coeff[7] = ((-20 * pf + 20 * p0) + (10 * vf + 10 * v0) * t - (  2 * af -   2 * a0) * t2) / t7;
    } break;
    default:
       assert(false && "Invalid degree of polynomial.");
    break;
    }
}

} // namespace dtCore