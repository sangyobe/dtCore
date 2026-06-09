/*!
 \file      dtRtLogDtMath.hpp
 \brief     dt::Math vector support for RtLog stream operator
 \author    myungjin.kim@hyundai.com
 \date      2026. 5. 28

 Include this header INSTEAD OF (or in addition to) dtRtLog.hpp in source files
 that log dt::Math vectors.  dtRtLog.hpp itself does not depend on dtMath.

 Supported types:
   dt::Math::Vector<N, T>    — fixed-size general vector
   dt::Math::Vector3<T, N>   — 3D vector (N defaults to 3)
   dt::Math::Vector4<T, N>   — 4D vector (N defaults to 4)
   dt::Math::Vector6<T, N>   — 6D vector (N defaults to 6)
   dt::Math::VectorX<T>      — dynamic-size vector

 Usage:
   #include <dtCore/src/dtLog/dtRtLogDtMath.hpp>
   ...
   dt::Math::Vector<6, float> q;
   LOG_RT(info) << "joint_pos=" << q;
   // → joint_pos=[0.100000, 0.200000, ...]
*/
#pragma once
#include "dtRtLog.hpp"
#include <dtMath/dtMath.h>

namespace dt {

template<uint16_t N, typename T>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const dt::Math::Vector<N, T>& vec) noexcept 
{
    if (!m_active)
    {
        return *this;
    }

    if (m_pos + 1 >= BUF_LEN)
    {
        return *this;
    }

    m_buf[m_pos++] = '[';

    for (uint16_t i = 0; i < N; ++i) 
    {
        if (!FormatElement(static_cast<double>(vec(i)))) 
        {
            AddTruncation();
            break;
        }
        
        if (i != N - 1) 
        {
            if (!AddSeparator())
            {
                break;
            }
        }
    }

    CloseArray();
    return *this;
}

template<typename T, uint16_t N>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const dt::Math::Vector3<T, N>& vec) noexcept 
{
    if (!m_active)
    {
        return *this;
    }

    if (m_pos + 1 >= BUF_LEN)
    {
        return *this;
    }

    m_buf[m_pos++] = '[';

    for (uint16_t i = 0; i < N; ++i) 
    {
        if (!FormatElement(static_cast<double>(vec(i)))) 
        {
            AddTruncation();
            break;
        }
        
        if (i != N - 1) 
        {
            if (!AddSeparator())
            {
                break;
            }
        }
    }

    CloseArray();
    return *this;
}

template<typename T, uint16_t N>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const dt::Math::Vector4<T, N>& vec) noexcept 
{
    if (!m_active)
    {
        return *this;
    }

    if (m_pos + 1 >= BUF_LEN)
    {
        return *this;
    }

    m_buf[m_pos++] = '[';

    for (uint16_t i = 0; i < N; ++i) 
    {
        if (!FormatElement(static_cast<double>(vec(i)))) 
        {
            AddTruncation();
            break;
        }

        if (i != N - 1) 
        {
            if (!AddSeparator())
            {
                break;
            }
        }
    }

    CloseArray();
    return *this;
}

template<typename T, uint16_t N>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const dt::Math::Vector6<T, N>& vec) noexcept 
{
    if (!m_active)
    {
        return *this;
    }

    if (m_pos + 1 >= BUF_LEN)
    {
        return *this;
    }

    m_buf[m_pos++] = '[';

    for (uint16_t i = 0; i < N; ++i) 
    {
        if (!FormatElement(static_cast<double>(vec(i)))) 
        {
            AddTruncation();
            break;
        }

        if (i != N - 1) 
        {
            if (!AddSeparator())
            {
                break;
            }
        }
    }

    CloseArray();
    return *this;
}

template<typename T>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const dt::Math::VectorX<T>& vec) noexcept 
{
    if (!m_active)
    {
        return *this;
    }

    if (m_pos + 1 >= BUF_LEN)
    {
        return *this;
    }

    m_buf[m_pos++] = '[';

    const uint16_t n = vec.GetDim();
    for (uint16_t i = 0; i < n; ++i) 
    {
        if (!FormatElement(static_cast<double>(vec(i)))) 
        {
            AddTruncation();
            break;
        }
        
        if (i != n - 1) 
        {
            if (!AddSeparator())
            {
                break;
            }
        }
    }

    CloseArray();
    return *this;
}

}  // namespace dt
