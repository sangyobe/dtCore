#include <chrono>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtPolynomialTrajectory;
using dtCore::dtTrajectoryList;

void PolynomialTrajectory1() {
  double ti = 0.0;
  double tf = 10.0;
  double pi[3] = {0.0, 5.0, -10.0};
  double pf[3] = {5.0, -5.0, 0.0};
  double pf2[3] = {0.0, -5.0, 5.0};
  double vi[3] = {0.0, 0.0, 0.0};
  double vf[3] = {0.0, 0.0, 0.0};
  double ai[3] = {0.0, 0.0, 0.0};
  double af[3] = {0.0, 0.0, 0.0};

  double td = tf - ti; // interpolation time duration
  double tc;
  double p[3], v[3], a[3]; // output

  // 7th-order
  dtPolynomialTrajectory<double, 3, 7> traj7;
  for (uint16_t i = 0; i < 2000; i++)
  {
    // robot command
    if (i == 0)
    {
      traj7.SetDuration(td);
      traj7.SetInitParam(pi, vi, ai);
      traj7.SetTargetParam(pf, vf, af);
      traj7.Configure();
    }

    if (i == 500)
    {
      traj7.SetParam(td, p, pf2, v, vf, a, af, 0.01*(i - 1));
      traj7.Configure();
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
  double pi[3] = { 0.0,  5.0, -10.0};
  double pf[3] = { 5.0, -5.0,   0.0};
  double vi[3] = {-1.0,  1.0,   0.0};
  double vf[3] = { 1.0,  0.0,  -2.0};
  double ai[3] = { 0.0,  0.0,   0.0};
  double af[3] = { 2.0,  4.0,   4.0};

  double td = tf - ti; // interpolation time duration
  double tc;
  double p[3], v[3], a[3]; // output

  // 7th-order
  dtPolynomialTrajectory<double, 3, 5> traj5(td, pi, pf, vi, vf, ai, af);
  for (uint16_t i = 0; i < 1001; i++)
  {
    // robot status
    traj5.Interpolate(0.01*i, p, v, a);
    std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2] << " " << v[0] << " " << v[1] << " " << v[2] << " " << a[0] << " " << a[1] << " " << a[2]
              << std::endl;
  }
}

void PolynomialTrajectory3()
{
  double ti = 0.0, t1 = 2.0, t2 = 7.0, tf = 10.0;
  double pi[3] = {0.0, 0.0, 0.0};
  double p1[3] = {0.5, 1.0, -0.5};
  double p2[3] = {0.7, 0.5, 0.0};
  double pf[3] = {1.0, 1.0, 1.0};
  double vi[3] = {0.0, 0.0, 0.0};
  double vf[3] = {0.0, 0.0, 0.0};
  double ai[3] = {0.0, 0.0, 0.0};
  double af[3] = {0.0, 0.0, 0.0};

  dtPolynomialTrajectory<double, 3, 3> traj1(t1 - ti, pi, p1);
  dtPolynomialTrajectory<double, 3, 3> traj2(t2 - t1, p1, p2);
  dtPolynomialTrajectory<double, 3, 3> traj3(tf - t2, p2, pf);
  dtTrajectoryList<dtPolynomialTrajectory<double, 3, 3>> trajList;
  trajList.Add(ti, traj1);
  trajList.Add(t1, traj2);
  trajList.Add(t2, traj3);

  double tc;
  double p[3], v[3], a[3]; // output

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- Test_PolynomialTrajectoryList -----------" << std::endl;
  tc = ti;
  for (int i = 0; i < 1001; i++)
  {
    trajList.Interpolate(0.01*i, p, v, a);
    std::cout << 0.01*i << " " << p[0] << " " << p[1] << " " << p[2]
              << std::endl;
    tc += 1.0;
  }
}

int main ()
{
    PolynomialTrajectory1();
    // PolynomialTrajectory2();
    // PolynomialTrajectory3();
}