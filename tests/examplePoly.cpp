#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtPolynomialTrajectory;
using dtCore::dtTrajectoryList;

void DoubleDof3Order5WithoutDefaultConstructor() 
{
    double ti = 1.0; //!< init time
    double td = 10.0; //!< trajectory duration
    double pi[3] = {0.0, 5.0, -10.0}; //!< init position
    double pf[3] = {5.0, -5.0, 0.0}; //!< target position
    double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
    double vf[3] = {0.0, 0.0, 0.0}; //!< target velocity
    double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
    double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
    double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

    dtPolynomialTrajectory<double, 3, 5> traj5(td, pi, pf, vi, vf, ai, af); //!< double, 3 dof, 7 order polynomial trajectory
                                                                            //!< Init time is set to zero because there is no input for ti.
    double controlPeriod = 0.01; //!< real time control period (sec)
    for (int i = 0; i < 1200; i++) //!< real time thread (0 ~ 12 sec)
    {
        double tc = 0.01*i; //!< current time
        traj5.Interpolate(tc, p, v); //!< input: tc, output: p, v
        std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2]
                  << std::endl;       
    }
}

void DoubleDof3Order7WithDefaultConstructor() 
{
  double ti = 1.0; //!< init time
  double td = 10.0; //!< trajectory duration
  double pi[3] = {0.0, 5.0, -10.0}; //!< init position
  double pf[3] = {5.0, -5.0, 0.0}; //!< target position
  double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
  double vf[3] = {0.0, 0.0, 0.0}; //!< target velocity
  double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
  double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
  double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

  dtPolynomialTrajectory<double, 3, 7> traj7; //!< double, 3 dof, 7 order polynomial trajectory
  double controlPeriod = 0.01; //!< real time control period (sec)
  for (int i = 0; i < 2000; i++) //!< real time thread (0 ~ 20 sec)
  {
      double tc = 0.01*i; //!< current time
      if (i == 0)
      {
          traj7.SetParam(td, pi, pf, vi, vf, ai, af, tc, ti);
          traj7.Configure(); //!< without this function, polynomial coefficeints are not configured.
      }

      traj7.Interpolate(tc, p, v, a); //!< input: tc, output: p, v, a
      std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
                << std::endl;       
    }
}

void DoubleDof3Order7WithDefaultConstructor2() 
{
    double td = 10.0; //!< trajectory duration
    double pi[3] = {0.0, 5.0, -10.0}; //!< init position
    double pf[3] = {5.0, -5.0, 0.0}; //!< target position
    double vi[3] = {0.0, 0.0, 0.0}; //!< init velocity
    double vf[3] = {0.0, 0.0, 0.0}; //!< target velocity
    double ai[3] = {0.0, 0.0, 0.0}; //!< init acceleration
    double af[3] = {0.0, 0.0, 0.0}; //!< target acceleration
    double p[3], v[3], a[3]; //!< trajectory output (p: desired position, v: desired velocity, a: desired acceleration)

    dtPolynomialTrajectory<double, 3, 7> traj7; //!< double, 3 dof, 7 order polynomial trajectory
    double controlPeriod = 0.01; //!< real time control period (sec)
    for (int i = 0; i < 2000; i++) //!< real time thread (0 ~ 20 sec)
    {
        double tc = 0.01*i; //!< current time
        if (i == 150)
        {
            traj7.SetDuration(td);
            traj7.SetInitParam(pi, vi, ai);
            traj7.SetTargetParam(pf, vf, af);
            traj7.SetTimeOffset(tc); //!< Set current time as trajectory init time. (ti = tc = 0.01*i = 1.5sec)
            traj7.Configure(); //!< without this function, polynomial coefficeints are not configured.
        }

        traj7.Interpolate(tc, p, v, a); //!< input: tc, output: p, v, a
        std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
                  << std::endl;       
    }
}

int main ()
{
    DoubleDof3Order5WithoutDefaultConstructor();
    // DoubleDof3Order7WithDefaultConstructor();
    // DoubleDof3Order7WithDefaultConstructor2();
}