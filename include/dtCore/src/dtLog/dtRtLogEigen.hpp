/*!
 \file      dtRtLogEigen.hpp
 \brief     Eigen dense matrix/vector support for RtLog stream operator
 \author    myungjin.kim@hyundai.com
 \date      2026. 5. 28

 Include this header INSTEAD OF (or in addition to) dtRtLog.hpp in source files
 that log Eigen vectors or matrices.  dtRtLog.hpp itself does not depend on Eigen.

 Supported types (anything derived from Eigen::MatrixBase):
   Eigen::VectorXf, VectorXd, VectorXi
   Eigen::Vector3f, Vector4d, ...
   Eigen::MatrixXf, MatrixXd, ...  (elements printed in column-major order)

 Usage:
   #include <dtCore/src/dtLog/dtRtLogEigen.hpp>
   ...
   Eigen::VectorXf q(6);
   LOG_RT(info) << "joint_pos=" << q;
   // → joint_pos=[0.100000, 0.200000, ...]
*/
#pragma once
#include "dtRtLog.hpp"
#include <eigen3/Eigen/Dense>

namespace dt {

template<typename Derived>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const Eigen::MatrixBase<Derived>& vec) noexcept 
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

    const Eigen::Index n = vec.size();
    for (Eigen::Index i = 0; i < n; ++i) 
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
