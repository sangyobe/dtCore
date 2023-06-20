#include <chrono>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtPolynomialTrajectory;

void PolynomialTrajectory1() {
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
  dtPolynomialTrajectory<double, 3, 7> traj7;
  for (int i = 0; i < 2000; i++)
  {
    // robot command
    if (i == 0)
    {
      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "- 7th-order polynomial --------------------" << std::endl;
      traj7.SetDuration(td);
      traj7.SetInitParam(pi, vi, ai);
      traj7.SetTargetParam(pf, vf, af);
      traj7.ReConfigure();
    }

    if (i == 500)
    {
      traj7.SetParam(td, p, pf, v, vf, a, af, 0.01*(i - 1));
      traj7.ReConfigure();
    }

    // robot status
    traj7.Interpolate(0.01*i, p, v, a);
    std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
              << std::endl;
              
  }
}

void PolynomialTrajectory2() {
  double ti = 0.0;
  double tf = 10.0;
  double pi[3] = {0.0, 5.0, -10.0};
  double pf[3] = {5.0, -5.0, 0.0};
  double vi[3] = {0.0, 0.0, 0.0};
  double vf[3] = {0.0, 0.0, 2.0};
  double ai[3] = {0.0, 0.0, 0.0};
  double af[3] = {0.0, 0.0, 0.0};

  double td = tf - ti; // interpolation time duration
  double tc;
  double p[3], v[3], a[3]; // output

  // 7th-order
  dtPolynomialTrajectory<double, 3, 5> traj5(td, pi, pf, vi, vf);
  for (int i = 0; i < 1001; i++)
  {
    // robot status
    traj5.Interpolate(0.01*i, p, v, a);
    std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
              << std::endl;
              
  }
}

int main ()
{
    PolynomialTrajectory1();
    // PolynomialTrajectory2();
}