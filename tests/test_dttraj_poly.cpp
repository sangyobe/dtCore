#include <gtest/gtest.h>
#include "dtCore/dtTrajectory"

namespace TestTrajectory
{
    namespace
    {
        TEST(PolynomialTrajectory, Initialize)
        {
            double ti = 1.0; //!< init time
            double td = 10.0; //!< trajectory duration
            double pi[3] = {0.0, 5.0, -10.0}; //!< init position
            double pf[3] = {5.0, -5.0, 0.0}; //!< target position
            double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
            double vf[3] = {0.0, 1.0, 0.0}; //!< target velocity
            double ai[3] = {0.0, 0.0, 1.0}; //!< init acceleration
            double af[3] = {0.0, 0.0, 1.0}; //!< target acceleration
            double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

            dtCore::dtPolynomialTrajectory<double, 3, 5> traj(td, pi, pf, vi, vf, ai, af, ti); //!< double, 3 dof, 5 order polynomial trajectory
                                                                                    //!< Init time is set to zero because there is no input for ti.
            double controlPeriod = 0.01; //!< real time control period (sec)
            for (int i = 0; i < 1200; i++) //!< real time thread (0 ~ 12 sec)
            {
                double tc = 0.01*i; //!< current time
                traj.Interpolate(tc, p, v); //!< input: tc, output: p, v
                std::cout << 0.01*i << ", " << p[0] << ", " << p[1] << ", " << p[2] << ", " << v[0] << ", " << v[1] << ", " << v[2]
                        << std::endl;       
            }
            traj.Interpolate(ti+td, p, v, a);
            EXPECT_DOUBLE_EQ(p[0], pf[0]);
            EXPECT_DOUBLE_EQ(p[1], pf[1]);
            EXPECT_DOUBLE_EQ(p[2], pf[2]);
            EXPECT_DOUBLE_EQ(v[1], vf[1]);
            EXPECT_DOUBLE_EQ(a[2], af[2]);
        }
    }
}