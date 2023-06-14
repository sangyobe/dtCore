#include <chrono>
#include <dtCore/dtCore>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtBezierTrajectory;
using dtCore::dtHTransformTrajectory;
using dtCore::dtOrientationTrajectory;
using dtCore::dtPolynomialTrajectory;
using dtCore::dtTrajectoryList;
using dtCore::dtVectorPolynomialTrajectory;

void Test_PolynomialTrajectory() {
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

  // 1st-order
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 1st-order polynomial --------------------" << std::endl;
  dtPolynomialTrajectory<double, 3, 1> traj1(td, pi, pf);
  traj1.SetTimeOffset(0.0);

  tc = ti;
  while (tc <= tf) {
    traj1.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }

  // 3rd-order
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 3rd-order polynomial --------------------" << std::endl;
  dtPolynomialTrajectory<double, 3, 3> traj3(td, pi, pf, vi, vf);
  // traj3.SetTimeOffset(0.0);

  tc = ti;
  while (tc <= tf) {
    traj3.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }

  // 5th-order
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 5th-order polynomial --------------------" << std::endl;
  dtPolynomialTrajectory<double, 3, 5> traj5(td, pi, pf, vi, vf, ai, af);
  // traj5.SetTimeOffset(0.0);

  tc = ti;
  while (tc <= tf) {
    traj5.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }
}

void Test_PolynomialTrajectoryList() {
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
  while (tc <= tf) {
    trajList.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }
}

void Test_BezierTrajectory() {
  double td = 10.0; // interpolation time duration
  double pi[3] = {0.0, 5.0, -10.0};
  double pf[3] = {5.0, -5.0, 0.0};
  double *pc1 = NULL;
  double pc2[3] = {20.0, -10.0, 10.0};
  double pc3[6] = {20.0, 20.0, -10.0, -10.0, 10.0, 10.0};
  double tc;
  double p[3], v[3], a[3]; // output

  // 1st-order bezier (linear)
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 1st-order bezier ------------------------" << std::endl;
  dtBezierTrajectory<double, 3, 1> traj1(td, pi, pf, pc1);
  for (tc = 0; tc <= td; tc += 1.0) {
    traj1.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
  }

  // 2nd-order bezier
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 2nd-order bezier ------------------------" << std::endl;
  dtBezierTrajectory<double, 3, 2> traj2(td, pi, pf, pc2);
  for (tc = 0; tc <= td; tc += 1.0) {
    traj2.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
  }

  // 3rd-order bezier
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 3rd-order bezier ------------------------" << std::endl;
  dtBezierTrajectory<double, 3, 3> traj3(td, pi, pf, pc3);
  for (tc = 0; tc <= td; tc += 1.0) {
    traj3.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
  }
}

void Test_BezierTrajectoryList() {
  double ti = 0.0, t1 = 2.0, t2 = 7.0, tf = 10.0;
  double pi[3] = {0.0, 0.0, 0.0};
  double p1[3] = {0.5, 1.0, -0.5};
  double p2[3] = {0.7, 0.5, 0.0};
  double pf[3] = {1.0, 1.0, 1.0};
  double pc1[3] = {1.0, 2.0, -1.0};
  double pc2[3] = {1.0, 0.0, 0.5};
  double pc3[6] = {1.0, 1.0, 1.0};

  dtBezierTrajectory<double, 3, 2> traj1(t1 - ti, pi, p1, pc1);
  dtBezierTrajectory<double, 3, 2> traj2(t2 - t1, p1, p2, pc2);
  dtBezierTrajectory<double, 3, 2> traj3(tf - t2, p2, pf, pc3);
  dtTrajectoryList<dtBezierTrajectory<double, 3, 2>> trajList;
  trajList.Add(ti, traj1);
  trajList.Add(t1, traj2);
  trajList.Add(t2, traj3);

  double tc;
  double p[3], v[3], a[3]; // output

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- Test_BezierTrajectoryList ---------------" << std::endl;
  tc = ti;
  while (tc <= tf) {
    trajList.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }
}

void Test_VectorPolynomialTrajectory() {
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

  // 1st-order
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 1st-order polynomial --------------------" << std::endl;
  dtPolynomialTrajectory<double, 3, 1> traj1(td, pi, pf, vi, vf, ai, af);
  traj1.SetTimeOffset(0.0);

  tc = ti;
  while (tc <= tf) {
    traj1.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }

  // 3rd-order
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 3rd-order polynomial --------------------" << std::endl;
  dtPolynomialTrajectory<double, 3, 3> traj3(td, pi, pf, vi, vf, ai, af);
  // traj3.SetTimeOffset(0.0);

  tc = ti;
  while (tc <= tf) {
    traj3.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }

  // 5th-order
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "- 5th-order polynomial --------------------" << std::endl;
  dtPolynomialTrajectory<double, 3, 5> traj5(td, pi, pf, vi, vf, ai, af);
  // traj5.SetTimeOffset(0.0);

  tc = ti;
  while (tc <= tf) {
    traj5.Interpolate(tc, p, v, a);
    std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2]
              << ")" << std::endl;
    tc += 1.0;
  }
}

void Test_VectorPolynomialTrajectoryList() {
  double ti = 0.0, t1 = 2.0, t2 = 7.0, tf = 10.0;
  dtMath::Vector3d pi, p1, p2, pf;
  pi << 0.0, 0.0, 0.0;
  p1 << 0.5, 1.0, -0.5;
  p2 << 0.7, 0.5, 0.0;
  pf << 1.0, 1.0, 1.0;
  dtVectorPolynomialTrajectory<double, 3> traj1(dtCore::dtPolyType::CUBIC, ti,
                                                t1, pi, p1);
  dtVectorPolynomialTrajectory<double, 3> traj2(dtCore::dtPolyType::CUBIC, t1,
                                                t2, p1, p2);
  dtVectorPolynomialTrajectory<double, 3> traj3(dtCore::dtPolyType::CUBIC, t2,
                                                tf, p2, pf);
  dtTrajectoryList<dtVectorPolynomialTrajectory<double, 3>> trajList;
  trajList.Add(ti, traj1);
  trajList.Add(t1, traj2);
  trajList.Add(t2, traj3);

  double tc;
  dtMath::Vector3d p, v, a;

  tc = 1.0;
  trajList.Interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 5.0;
  trajList.Interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 9.0;
  trajList.Interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;
}

void Test_OrientationTrajectoryList() {
  double ti = 0.0, t1 = 2.0, t2 = 7.0, tf = 10.0;
  dtMath::RotationMatrix Ri, R1, R2, Rf;
  Ri << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
  R1 << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
  R2 << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
  Rf << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
  dtOrientationTrajectory<double> traj1(ti, t1, Ri, R1);
  dtOrientationTrajectory<double> traj2(t1, t2, R1, R2);
  dtOrientationTrajectory<double> traj3(t2, tf, R2, Rf);
  dtTrajectoryList<dtOrientationTrajectory<double>> trajList;
  trajList.Add(ti, traj1);
  trajList.Add(t1, traj2);
  trajList.Add(t2, traj3);

  double tc;
  dtMath::RotationMatrix Rc;

  tc = 1.0;
  trajList.Interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 5.0;
  trajList.Interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 9.0;
  trajList.Interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;
}

void Test_OrientationTrajectory() {
  double ti = 0.0;
  double tf = 10.0;
  dtMath::RotationMatrix Ri, Rf;
  Ri << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
  Rf << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
  dtOrientationTrajectory<double> traj(ti, tf, Ri, Rf);

  double tc;
  dtMath::RotationMatrix Rc;

  tc = 1.0;
  traj.Interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 5.0;
  traj.Interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 9.0;
  traj.Interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;
}

int main() {
  Test_PolynomialTrajectory();
  Test_BezierTrajectory();
  Test_PolynomialTrajectoryList();
  Test_BezierTrajectoryList();
  return 0;
}