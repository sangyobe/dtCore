#include <chrono>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtScurveTrajectory;

class Duration //!< Example of creating an S-curve trajectory using trajectory duration, trajectory acceleration duration
{
    public:
    void DoubleDof3Order5WithoutDefaultConstructor() 
    {
        double ti = 1.0; //!< init time
        double td = 10.0; //!< trajectory duration
        double ta = 2.0; //!< trajectory acceleration duration
        double pi[3] = {0.0, 5.0, -10.0}; //!< init position
        double pf[3] = {5.0, -5.0, 0.0}; //!< target position
        double vi[3] = {1.0, 0.0, 0.0}; //!< init velocity
        double vf[3] = {0.0, 1.0, 0.0}; //!< target velocity
        double ai[3] = {0.0, 0.0, 1.0}; //!< init acceleration
        double af[3] = {0.0, 0.0, 1.0}; //!< target acceleration
        double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

        dtScurveTrajectory<double, 3, 5> traj5(td, ta, pi, pf, vi, vf, ai, af); //!< double, 3 dof, 5 order S-curve trajectory
                                                                                //!< Init time is set to zero because there is no input for ti.
        double controlPeriod = 0.01; //!< real time control period (sec)
        for (int i = 0; i < 1200; i++) //!< real time thread (0 ~ 12 sec)
        {
            double tc = 0.01*i; //!< current time
            traj5.Interpolate(tc, p, v); //!< input: tc, output: p, v
            std::cout << 0.01*i << ", " << p[0] << ", " << p[1] << ", " << p[2] << ", " << v[0] << ", " << v[1] << ", " << v[2]
                      << std::endl;       
        }
    }

    void DoubleDof3Order7WithDefaultConstructor() 
    {
      double ti = 1.0; //!< init time
      double td = 10.0; //!< trajectory duration
      double ta = 2.0; //!< trajectory acceleration duration
      double pi[3] = {0.0, 5.0, -10.0}; //!< init position
      double pf[3] = {5.0, -5.0, 0.0}; //!< target position
      double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
      double vf[3] = {0.0, 0.0, 0.0}; //!< target velocity
      double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
      double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
      double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

      dtScurveTrajectory<double, 3, 7> traj7; //!< double, 3 dof, 7 order S-curve trajectory
      double controlPeriod = 0.01; //!< real time control period (sec)
      for (int i = 0; i < 1200; i++) //!< real time thread (0 ~ 20 sec)
      {
          double tc = 0.01*i; //!< current time
          if (i == 0)
          {
              traj7.SetParam(td, ta, pi, pf, vi, vf, ai, af, ti);
              traj7.Configure(); //!< without this function, polynomial coefficeints are not configured.
          }

          traj7.Interpolate(tc, p, v, a); //!< input: tc, output: p, v, a
          std::cout << 0.01*i << ", " << p[0] << ", " << p[1] << ", " << p[2] << ", " << v[0] << ", " << v[1] << ", " << v[2] << ", " << a[0] << ", " << a[1] << ", " << a[2]
                    << std::endl;       
        }
    }

    void DoubleDof3Order7WithDefaultConstructor2() 
    {
        double td = 8.0; //!< trajectory duration
        double ta = 1.5; //!< trajectory duration
        double pi[3] = {0.0, 5.0, -10.0}; //!< init position
        double pf[3] = {5.0, -5.0, 0.0}; //!< target position
        double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
        double vf[3] = {0.0, 0.0, 0.0}; //!< target velocity
        double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
        double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
        double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

        dtScurveTrajectory<double, 3, 7> traj7; //!< double, 3 dof, 7 order S-curve trajectory
        double controlPeriod = 0.01; //!< real time control period (sec)
        for (int i = 0; i < 1300; i++) //!< real time thread (0 ~ 20 sec)
        {
            double tc = 0.01*i; //!< current time
            if (i == 150)
            {
                traj7.SetDuration(td, ta);
                traj7.SetInitParam(pi, vi, ai);
                traj7.SetTargetParam(pf, vf, af);
                traj7.SetTimeOffset(tc); //!< Set current time as trajectory init time. (ti = tc = 0.01*i = 1.5sec)
                traj7.Configure(); //!< without this function, polynomial coefficeints are not configured.
            }

            traj7.Interpolate(tc, p, v, a); //!< input: tc, output: p, v, a
            std::cout << 0.01*i << ", " << p[0] << ", " << p[1] << ", " << p[2] << ", " << v[0] << ", " << v[1] << ", " << v[2] << ", " << a[0] << ", " << a[1] << ", " << a[2]
                      << std::endl;       
        }
    }
};

class Limit //!< Example of creating an S-curve trajectory using velocity limit, acceleration limit
{
    public:
    void DoubleDof3Order5WithoutDefaultConstructor() 
    {
        double ti = 1.0; //!< init time
        double vl[3] = {1.0, 1.0, 1.0}; //!< velocity limit
        double al[3] = {1.0, 1.0, 1.0}; //!< acceleration limit
        double pi[3] = {0.0, 5.0, -10.0}; //!< init position
        double pf[3] = {5.0, -5.0, 0.0}; //!< target position
        double vi[3] = {1.0, 0.0, 0.0}; //!< init velocity
        double vf[3] = {0.0, 1.0, 0.0}; //!< target velocity
        double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
        double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
        double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

        dtScurveTrajectory<double, 3, 5> traj5(vl, al, pi, pf, vi, vf, ai, af); //!< double, 3 dof, 5 order S-curve trajectory
                                                                                //!< Init time is set to zero because there is no input for ti.
        double controlPeriod = 0.01; //!< real time control period (sec)
        for (int i = 0; i < 2000; i++) //!< real time thread (0 ~ 12 sec)
        {
            double tc = 0.01*i; //!< current time
            traj5.Interpolate(tc, p, v, a); //!< input: tc, output: p, v
            std::cout << 0.01*i << ", " << p[0] << ", " << p[1] << ", " << p[2] << ", " << v[0] << ", " << v[1] << ", " << v[2] << ", " << a[0] << ", " << a[1] << ", " << a[2]
                      << std::endl;       
        }
    }

    void DoubleDof3Order7WithDefaultConstructor() 
    {
        double ti = 1.0; //!< init time
        double vl[3] = {1.0, 1.0, 1.0}; //!< velocity limit
        double al[3] = {1.0, 1.0, 1.0}; //!< acceleration limit
        double pi[3] = {0.0, 5.0, -10.0}; //!< init position
        double pf[3] = {5.0, -5.0, 0.0}; //!< target position
        double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
        double vf[3] = {0.0, 0.0, 0.0}; //!< target velocity
        double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
        double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
        double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

        dtScurveTrajectory<double, 3, 7> traj7; //!< double, 3 dof, 7 order S-curve trajectory
        double controlPeriod = 0.01; //!< real time control period (sec)
        for (int i = 0; i < 2000; i++) //!< real time thread (0 ~ 20 sec)
        {
            double tc = 0.01*i; //!< current time
            if (i == 0)
            {
                traj7.SetParam(vl, al, pi, pf, vi, vf, ai, af, ti);
                traj7.Configure(); //!< without this function, polynomial coefficeints are not configured.
            }

            traj7.Interpolate(tc, p, v, a); //!< input: tc, output: p, v, a
            std::cout << 0.01*i << ", " << p[0] << ", " << p[1] << ", " << p[2] << ", " << v[0] << ", " << v[1] << ", " << v[2] << ", " << a[0] << ", " << a[1] << ", " << a[2]
                      << std::endl;       
        }
    }

    void DoubleDof3Order7WithDefaultConstructor2() 
    {
        double vl[3] = {1.0, 1.0, 1.0}; //!< velocity limit
        double al[3] = {1.0, 1.0, 1.0}; //!< acceleration limit
        double pi[3] = {0.0, 5.0, -10.0}; //!< init position
        double pf[3] = {5.0, -5.0, 0.0}; //!< target position
        double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
        double vf[3] = {0.0, 0.0, 0.0}; //!< target velocity
        double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
        double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
        double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

        dtScurveTrajectory<double, 3, 7> traj7; //!< double, 3 dof, 7 order S-curve trajectory
        double controlPeriod = 0.01; //!< real time control period (sec)
        for (int i = 0; i < 1800; i++) //!< real time thread (0 ~ 20 sec)
        {
            double tc = 0.01*i; //!< current time
            if (i == 150)
            {
                traj7.SetLimit(vl, al);
                traj7.SetInitParam(pi, vi, ai);
                traj7.SetTargetParam(pf, vf, af);
                traj7.SetTimeOffset(tc); //!< Set current time as trajectory init time. (ti = tc = 0.01*i = 1.5sec)
                traj7.Configure(); //!< without this function, polynomial coefficeints are not configured.
            }

            traj7.Interpolate(tc, p, v, a); //!< input: tc, output: p, v, a
            std::cout << 0.01*i << ", " << p[0] << ", " << p[1] << ", " << p[2] << ", " << v[0] << ", " << v[1] << ", " << v[2] << ", " << a[0] << ", " << a[1] << ", " << a[2]
                      << std::endl;       
        }
    }
};

int main ()
{
    //!< Example of four ways to generate S-curve trajectories

    //// Example of creating an S-curve trajectory using trajectory duration, trajectory acceleration duration
    Duration duration;
    // duration.DoubleDof3Order5WithoutDefaultConstructor();
    // duration.DoubleDof3Order7WithDefaultConstructor();
    // duration.DoubleDof3Order7WithDefaultConstructor2();

    //// Example of creating an S-curve trajectory using velocity limit, acceleration limit
    Limit limit;
    // limit.DoubleDof3Order5WithoutDefaultConstructor();
    // limit.DoubleDof3Order7WithDefaultConstructor();
    limit.DoubleDof3Order7WithDefaultConstructor2();



}