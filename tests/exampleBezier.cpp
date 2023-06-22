#include <chrono>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtBezierTrajectory;

void BezierTrajectory() 
{
    double td = 10.0; // interpolation time duration
    double pi[3]    = { 0.0,  0.0,   0.0};
    double vi[3]    = { 0.0,  5.0,   5.0};
    double ai[3]    = { 0.0,  0.0,   0.0};
    double pc3[5*3] = {20.0, 20.0, -10.0,
                        0.0,  0.0,  30.0,
                       15.0, 10.0,  10.0,
                       10.0, 10.0,  10.0,
                        0.0, 10.0,  10.0};
    double af[3]    = { 0.0,  0.0,   0.0};
    double vf[3]    = {10.0,  0.0,  -5.0};
    double pf[3]    = { 5.0, -5.0,   0.0};
    double tc;
    double p[3], v[3], a[3];
    //TODO: 2차원 배열 받게하는거?

    //TODO: contorl point 넣는 방법 주석처리
    dtBezierTrajectory<double, 3, 10> traj3(10, pi, pf, vi, vf, ai, af, pc3, 5);

    for (tc = 0; tc <= td; tc += 0.001) 
    {
        traj3.Interpolate(tc, p, v, a);
        std::cout << tc << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2] << std::endl;
    }
}

void BezierTrajectory2() 
{
    double td = 10.0; // interpolation time duration
    double pi[3]    = { 0.0,    0.0, 0.0};
    double vi[3]    = { 10.0, -10.0, 10.0};
    double ai[3]    = { 0.0,    0.0, 0.0};

    double pf[3]    = { 60.0, 30.0, 40.0};
    double vf[3]    = { 5.0,  5.0,  -5.0};
    double af[3]    = { 0.0,  0.0,   0.0};

    double pc3[10*3] = {0, -3, 24,
                       0, -3, 24,
                       0, -3, 40,
                       30, 15, 80,
                       30, 15, 88,
                       30, 15, 88,
                       30, 15, 80,
                       60, 39, 84,
                       60, 39, 60,
                       60, 39, 60,
                    };
    double tc;
    double p[3], v[3], a[3];
    //TODO: 2차원 배열 받게하는거?

    //TODO: contorl point 넣는 방법 주석처리
    dtBezierTrajectory<double, 3, 10> traj3(10, pi, pf, vi, vf, ai, af, pc3, 10);

    for (tc = 0; tc <= td; tc += 0.001) 
    {
        traj3.Interpolate(tc, p, v, a);
        std::cout << tc << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2] << std::endl;
    }
}

int main ()
{
    BezierTrajectory();
}