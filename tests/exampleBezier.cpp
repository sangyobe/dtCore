#include <chrono>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtBezierTrajectory;

void DoubleDof3WithoutDefaultConstructor() 
{
    double ti = 1.0; //!< init time
    double td = 10.0; //!< trajectory duration
    double pi[3]   = { 0.0,  0.0,   0.0}; //!< init position
    double pf[3]   = { 5.0, -5.0,   0.0}; //!< target position
    double vi[3]   = { 0.0,  5.0,   5.0}; //!< init velocity
    double vf[3]   = {10.0,  0.0,  -5.0}; //!< target velocity
    double ai[3]   = { 0.0,  0.0,   0.0}; //!< init acceleration
    double af[3]   = { 0.0,  0.0,   0.0}; //!< target acceleration
    double pc[5*3] = {20.0, 20.0, -10.0, //!< 1-st control point {20.0, 20.0, -10.0}
                       0.0,  0.0,  30.0, //!< 2-nd control point { 0.0,  0.0,  30.0}
                      15.0, 10.0,  10.0, //!< 3-rd control point {15.0, 10.0,  10.0}
                      10.0, 10.0,  10.0, //!< 4-th control point {10.0, 10.0,  10.0}
                       0.0, 10.0,  10.0  //!< 5-th control point { 0.0, 10.0,  10.0}
                     }; //!< input control point
    double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

    dtBezierTrajectory<double, 3, 10> traj(td, pi, pf, vi, vf, ai, af, pc, ti); // double, 3 dof, max 10 input control point num bezier trajectory

    for (int i = 0; i < 1200; i++) //!< real time thread (0 ~ 12 sec)
    {
        double tc = 0.01*i; //!< current time
        traj.Interpolate(tc, p, v); //!< input: tc, output: p, v
        std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2]
                  << std::endl;
    }
}

void DoubleDof3WithDefaultConstructor() 
{
    double ti = 1.0; //!< init time
    double td = 10.0; // interpolation time duration
    double pi[3]     = {  0.0,   0.0,  0.0}; //!< init position
    double vi[3]     = { 10.0, -10.0, 10.0}; //!< target position
    double ai[3]     = {  0.0,   0.0,  0.0}; //!< init velocity
    double pf[3]     = { 60.0,  30.0, 40.0}; //!< target velocity
    double vf[3]     = {  5.0,   5.0, -5.0}; //!< init acceleration
    double af[3]     = {  0.0,   0.0,  0.0}; //!< target acceleration
    double pc[10][3] = {{ 0.0,  -3.0, 24.0}, //!<  1-st control point { 0.0,  -3.0, 24.0}
                        { 0.0,  -3.0, 24.0}, //!<  2-nd control point { 0.0,  -3.0, 24.0}
                        { 0.0,  -3.0, 40.0}, //!<  3-rd control point { 0.0,  -3.0, 40.0}
                        {30.0,  15.0, 80.0}, //!<  4-th control point {30.0,  15.0, 80.0}
                        {30.0,  15.0, 88.0}, //!<  5-th control point {30.0,  15.0, 88.0}
                        {30.0,  15.0, 88.0}, //!<  6-th control point {30.0,  15.0, 88.0}
                        {30.0,  15.0, 80.0}, //!<  7-th control point {30.0,  15.0, 88.0}
                        {60.0,  39.0, 84.0}, //!<  8-th control point {60.0,  39.0, 84.0}
                        {60.0,  39.0, 60.0}, //!<  9-th control point {60.0,  39.0, 60.0}
                        {60.0,  39.0, 60.0}, //!< 10-th control point {60.0,  39.0, 60.0}
                       }; //!< input control point
    uint16_t pcNum = 10;
    double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

    dtBezierTrajectory<double, 3, 11> traj; // double, 3 dof, max 10 input control point num bezier trajectory
    double controlPeriod = 0.01; //!< real time control period (sec)
    for (int i = 0; i < 2000; i++) //!< real time thread (0 ~ 20 sec)
    {
        double tc = controlPeriod*i; //!< current time
        if (i == 0)
        {
            traj.SetParam(td, pi, pf, vi, vf, ai, af, *pc, pcNum);
            traj.Configure(); //!< without this function, polynomial coefficeints are not configured.
        }

        traj.Interpolate(tc, p, v, a); //!< input: tc, output: p, v, a
        std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
                  << std::endl;       
    }
}

int main ()
{
    // DoubleDof3WithoutDefaultConstructor();
    DoubleDof3WithDefaultConstructor();
}