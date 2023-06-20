namespace dtCore {

template <typename ValueType, uint16_t m_order>
dtScurve<ValueType, m_order>::dtScurve()
{
  static_assert(m_order == 1 || m_order == 3 || m_order == 5 || m_order == 7,
                "Invalid degree of polynomial.");
}

template <typename ValueType, uint16_t m_order>
dtScurve<ValueType, m_order>::~dtScurve() {}

template <typename ValueType, uint16_t m_order>
void dtScurve<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p) const 
{
    ValueType coeff[m_order + 1] = {0.0, };
    ValueType t_ = 0;
    if (t < m_accDuration)
    {
        t_ = t;
        memcpy(coeff, m_accCoeff, (m_order + 1) * sizeof(ValueType));
    }
    else if (m_accDuration <= t && t < m_duration - m_decDuration)
    {
        t_ = t - m_accDuration;
        memcpy(coeff, m_conCoeff, (m_order + 1) * sizeof(ValueType));
    }
    else
    {
        t_ = t - (m_duration - m_decDuration);
        memcpy(coeff, m_decCoeff, (m_order + 1) * sizeof(ValueType));
    }

    switch (m_order) {
    case 1: 
    {
        p = coeff[0] + coeff[1] * t_;
    } break;

    case 3: 
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        p =     coeff[0]      + coeff[1] * t_
          +     coeff[2] * t2 + coeff[3] * t3;
    } break;

    case 5: 
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        const ValueType t4 = t_ * t3;
        const ValueType t5 = t_ * t4;
        p =      coeff[0]      +      coeff[1] * t_ 
          +      coeff[2] * t2 +      coeff[3] * t3
          +      coeff[4] * t4 +      coeff[5] * t5;
    } break;

    case 7:
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        const ValueType t4 = t_ * t3;
        const ValueType t5 = t_ * t4;
        const ValueType t6 = t_ * t5;
        const ValueType t7 = t_ * t6;
        p =      coeff[0]      +      coeff[1] * t 
          +      coeff[2] * t2 +      coeff[3] * t3
          +      coeff[4] * t4 +      coeff[5] * t5
          +      coeff[6] * t6 +      coeff[7] * t7;
    } break;

    default:
        // assert(false, "Invalid degree of polynomial.");
        break;
    }
}

template <typename ValueType, uint16_t m_order>
void dtScurve<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p, ValueType &v) const 
{
    ValueType coeff[m_order + 1] = {0.0, };
    ValueType t_ = 0;
    if (t < m_accDuration)
    {
        t_ = t;
        memcpy(coeff, m_accCoeff, (m_order + 1) * sizeof(ValueType));
    }
    else if (m_accDuration <= t && t < m_duration - m_decDuration)
    {
        t_ = t - m_accDuration;
        memcpy(coeff, m_conCoeff, (m_order + 1) * sizeof(ValueType));
    }
    else
    {
        t_ = t - (m_duration - m_decDuration);
        memcpy(coeff, m_decCoeff, (m_order + 1) * sizeof(ValueType));
    }

    switch (m_order) {
    case 1: 
    {
        p = coeff[0] + coeff[1] * t_;
        v = coeff[1];
    } break;

    case 3: 
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        p =     coeff[0]      +     coeff[1] * t_
          +     coeff[2] * t2 +     coeff[3] * t3;
        v =     coeff[1]      + 2 * coeff[2] * t_ 
          + 3 * coeff[3] * t2;
    } break;

    case 5: 
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        const ValueType t4 = t_ * t3;
        const ValueType t5 = t_ * t4;
        p =      coeff[0]      +      coeff[1] * t_ 
          +      coeff[2] * t2 +      coeff[3] * t3
          +      coeff[4] * t4 +      coeff[5] * t5;
        v =      coeff[1]      +  2 * coeff[2] * t_
          +  3 * coeff[3] * t2 +  4 * coeff[4] * t3 
          +  5 * coeff[5] * t4;
    } break;

    case 7:
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        const ValueType t4 = t_ * t3;
        const ValueType t5 = t_ * t4;
        const ValueType t6 = t_ * t5;
        const ValueType t7 = t_ * t6;
        p =      coeff[0]      +      coeff[1] * t_ 
          +      coeff[2] * t2 +      coeff[3] * t3
          +      coeff[4] * t4 +      coeff[5] * t5
          +      coeff[6] * t6 +      coeff[7] * t7;
        v =      coeff[1]      +  2 * coeff[2] * t_
          +  3 * coeff[3] * t2 +  4 * coeff[4] * t3 
          +  5 * coeff[5] * t4 +  4 * coeff[6] * t5 
          +  7 * coeff[7] * t6;
    } break;

    default:
        // assert(false, "Invalid degree of polynomial.");
        break;
    }
}

template <typename ValueType, uint16_t m_order>
void dtScurve<ValueType, m_order>::Interpolate(const ValueType t, ValueType &p, ValueType &v, ValueType &a) const 
{
    ValueType coeff[m_order + 1] = {0.0, };
    ValueType t_ = 0;
    if (t < m_accDuration)
    {
        t_ = t;
        memcpy(coeff, m_accCoeff, (m_order + 1) * sizeof(ValueType));
    }
    else if (m_accDuration <= t && t < m_duration - m_decDuration)
    {
        t_ = t - m_accDuration;
        memcpy(coeff, m_conCoeff, (m_order + 1) * sizeof(ValueType));
    }
    else
    {
        t_ = t - (m_duration - m_decDuration);
        memcpy(coeff, m_decCoeff, (m_order + 1) * sizeof(ValueType));
    }
        
    switch (m_order) {
    case 1: 
    {
        p = coeff[0] + coeff[1] * t_;
        v = coeff[1];
        a = 0.0;
    } break;

    case 3: 
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        p =     coeff[0]      +     coeff[1] * t_
          +     coeff[2] * t2 +     coeff[3] * t3;
        v =     coeff[1]      + 2 * coeff[2] * t_ 
          + 3 * coeff[3] * t2;
        a = 2 * coeff[2] + 3 * coeff[3] * t_;
    } break;

    case 5: 
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        const ValueType t4 = t_ * t3;
        const ValueType t5 = t_ * t4;
        p =      coeff[0]      +      coeff[1] * t_ 
          +      coeff[2] * t2 +      coeff[3] * t3
          +      coeff[4] * t4 +      coeff[5] * t5;
        v =      coeff[1]      +  2 * coeff[2] * t_
          +  3 * coeff[3] * t2 +  4 * coeff[4] * t3 
          +  5 * coeff[5] * t4;
        a =  2 * coeff[2]      +  6 * coeff[3] * t_ 
          + 12 * coeff[4] * t2 + 20 * coeff[5] * t3;
    } break;

    case 7:
    {
        const ValueType t2 = t_ * t_;
        const ValueType t3 = t_ * t2;
        const ValueType t4 = t_ * t3;
        const ValueType t5 = t_ * t4;
        const ValueType t6 = t_ * t5;
        const ValueType t7 = t_ * t6;
        p =      coeff[0]      +      coeff[1] * t_ 
          +      coeff[2] * t2 +      coeff[3] * t3
          +      coeff[4] * t4 +      coeff[5] * t5
          +      coeff[6] * t6 +      coeff[7] * t7;
        v =      coeff[1]      +  2 * coeff[2] * t_
          +  3 * coeff[3] * t2 +  4 * coeff[4] * t3 
          +  5 * coeff[5] * t4 +  6 * coeff[6] * t5 
          +  7 * coeff[7] * t6;
        a =  2 * coeff[2]      +  6 * coeff[3] * t_ 
          + 12 * coeff[4] * t2 + 20 * coeff[5] * t3
          + 30 * coeff[6] * t4 + 42 * coeff[7] * t5;
    } break;

    default:
        // assert(false, "Invalid degree of polynomial.");
        break;
    }
}

template <typename ValueType, uint16_t m_order>
void dtScurve<ValueType, m_order>::Configure(const ValueType p0, const ValueType pf, 
                                             const ValueType v0, const ValueType vf, 
                                             const ValueType a0, const ValueType af,
                                             const ValueType vm, const ValueType duration, 
                                             const ValueType accDuration, const ValueType decDuration) 
{
    m_duration = duration;
    m_accDuration = accDuration;
    m_decDuration = decDuration;
    
    const ValueType pa = p0 + 0.5 * (v0 + vm) * accDuration;
    const ValueType pd = pf - 0.5 * (vm + vf) * decDuration;

    Coefficient(p0, pa, v0, vm, a0,  0, accDuration, m_accCoeff);
    Coefficient(pa, pd, vm, vm,  0,  0, duration - accDuration - decDuration, m_conCoeff);
    Coefficient(pd, pf, vm, vf,  0, af, decDuration, m_decCoeff);
}

template <typename ValueType, uint16_t m_order>
void dtScurve<ValueType, m_order>::Coefficient(const ValueType p0, const ValueType pf,
                                               const ValueType v0, const ValueType vf,
                                               const ValueType a0, const ValueType af,
                                               const ValueType t, ValueType *coeff)
{
    switch (m_order) {
    case 1:
    {
        if (t > m_tolerance)
        {
            coeff[0] = p0;
            coeff[1] = (pf - p0) / t;
        }
        else
        {
            coeff[0] = p0;
            coeff[1] = 0;
        }
    } break;
    case 3:
    {
        if (t > m_tolerance)
        {
            const ValueType t2 = t * t;
            const ValueType t3 = t * t2;
            coeff[0] = p0;
            coeff[1] = v0;
            coeff[2] = (( 3 * pf - 3 * p0) - (vf + 2 * v0) * t) / t2;
            coeff[3] = ((-2 * pf + 2 * p0) + (vf +     v0) * t) / t3;
        }
        else
        {
            coeff[0] = p0;
            coeff[1] = v0;
            coeff[2] = 0;
            coeff[3] = 0;
        }
    } break;
    case 5:
    {
        if (t > m_tolerance)
        {
            const ValueType t2 = t * t;
            const ValueType t3 = t * t2;
            const ValueType t4 = t * t3;
            const ValueType t5 = t * t4;
            coeff[0] = p0;
            coeff[1] = v0;
            coeff[2] = 0.5 * a0;
            coeff[3] = (( 10 * pf - 10 * p0) - (4 * vf + 6 * v0) * t + (0.5 * af - 1.5 * a0) * t2) / t3;
            coeff[4] = ((-15 * pf + 15 * p0) + (7 * vf + 8 * v0) * t - (      af - 1.5 * a0) * t2) / t4;
            coeff[5] = ((  6 * pf -  6 * p0) - (3 * vf + 3 * v0) * t + (0.5 * af - 0.5 * a0) * t2) / t5;
        }
        else
        {
            coeff[0] = p0;
            coeff[1] = v0;
            coeff[2] = 0;
            coeff[3] = 0;
            coeff[4] = 0;
            coeff[5] = 0;
        }
    } break;
    case 7:
    {
        if (t > m_tolerance)
        {
            const ValueType t2 = t * t;
            const ValueType t3 = t * t2;
            const ValueType t4 = t * t3;
            const ValueType t5 = t * t4;
            const ValueType t6 = t * t5;
            const ValueType t7 = t * t6;
            coeff[0] = p0;
            coeff[1] = v0;
            coeff[2] = 0.5 * a0;
            coeff[3] = 0;
            coeff[4] = (( 35 * pf - 35 * p0) - (15 * vf + 20 * v0) * t + (2.5 * af - 5.0 * a0) * t2) / t4;
            coeff[5] = ((-84 * pf + 84 * p0) + (39 * vf + 45 * v0) * t - (  7 * af -  10 * a0) * t2) / t5;
            coeff[6] = ( (70 * pf - 70 * p0) - (34 * vf + 36 * v0) * t + (6.5 * af - 7.5 * a0) * t2) / t6;
            coeff[7] = ((-20 * pf + 20 * p0) + (10 * vf + 10 * v0) * t - (  2 * af -   2 * a0) * t2) / t7;
        }
        else
        {
            coeff[0] = p0;
            coeff[1] = v0;
            coeff[2] = 0;
            coeff[3] = 0;
            coeff[4] = 0;
            coeff[5] = 0;
            coeff[6] = 0;
            coeff[7] = 0;
        }
    } break;
    default:
      // assert(false, "Invalid degree of polynomial.");
      break;
    }
}

} // namespace dtCore