#include <chrono>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtBezierTrajectory;

void BezierTrajectory() 
{
    double td = 10.0; // interpolation time duration
    double pi[3] = {0.0, 5.0, -10.0};
    double vi[3] = {0.0, 5.0,  5.0};
    double ai[3] = {0.0, 0.0,  0.0};
    double pc3[3*5] = {20.0, 20.0, -10.0,
                    0.0,  0.0, 30.0,
                    15.0, 10.0, 10.0,
                    10.0, 10.0, 10.0,
                    0.0, 10.0, 10.0};
    double af[3] = {0.0, 0.0,  0.0};
    double vf[3] = {10.0, 0.0, -5.0};
    double pf[3] = {5.0, -5.0, 0.0};
    double tc;
    double p[3], v[3], a[3];

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "- nth-order bezier ------------------------" << std::endl;

    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    dtBezierTrajectory<double, 3, 10> traj3(10, pi, pf, vi, vf, ai, af, pc3, 5);

    for (tc = 0; tc <= td; tc += 0.001) 
    {
        traj3.Interpolate(tc, p, v, a);
        std::cout << tc << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2] << std::endl;
    }
    std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
    std::chrono::nanoseconds nano = end - start;
    std::cout << nano.count() << std::endl;
}

int main ()
{
    BezierTrajectory();
}