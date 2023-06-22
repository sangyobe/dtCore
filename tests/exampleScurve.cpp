#include <chrono>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtScurveTrajectory;

void ScurveTrajectory1() {
  double ti = 0.0;
  double tf = 10.0;
  double pi[3] = {0.0, 5.0, -10.0};
  double pf[3] = {5.0, -5.0, 0.0};
  double vi[3] = {0.0, 0.0, 0.0};
  double vf[3] = {0.0, 0.0, 0.0};
  double ai[3] = {0.0, 0.0, 0.0};
  double af[3] = {0.0, 0.0, 0.0};

  double td = tf - ti; // interpolation time duration
  double tc;
  double p[3], v[3], a[3]; // output

  // 7th-order
  dtScurveTrajectory<double, 3, 7> traj7;
  for (uint16_t i = 0; i < 2000; i++)
  {
    // robot command
    if (i == 0)
    {
      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "- 5th-order s-curve -----------------------" << std::endl;
      traj7.SetDuration(td, 0.2*td);
      traj7.SetInitParam(pi, vi, ai);
      traj7.SetTargetParam(pf, vf, af);
      traj7.Configure();
    }

    if (i == 500)
    {
      traj7.SetParam(td, 0.2*td, p, pf, v, vf, a, af, 0.01*(i - 1));
      traj7.Configure();
    }

    // robot status
    traj7.Interpolate(0.01*i, p, v, a);
    std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
              << std::endl;
              
  }
}

void ScurveTrajectory2() {
  double ti = 0.0;
  double tf = 10.0;
  double pi[3] = {0.0, 5.0, -10.0};
  double pf[3] = {5.0, -5.0, 0.0};
  double vi[3] = {0.0, 0.0, 0.0};
  double vf[3] = {0.0, 0.0, 0.0};
  double ai[3] = {0.0, 0.0, 0.0};
  double af[3] = {0.0, 0.0, 0.0};
  double vLimit[3] = {1.0, 1.0, 1.0};
  double aLimit[3] = {1.0, 1.0, 1.0};

  double td = tf - ti; // interpolation time duration
  double tc;
  double p[3], v[3], a[3]; // output

  // 5th-order
  dtScurveTrajectory<double, 3, 5> traj5(vLimit, aLimit, pi, pf, vi, vf, ai, af);
  for (uint16_t i = 0; i < 2000; i++)
  {
    // robot status
    traj5.Interpolate(0.01*i, p, v, a);
    std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
              << std::endl;
              
  }
}

int main ()
{
    // ScurveTrajectory1();
    ScurveTrajectory2();
}