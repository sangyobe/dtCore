#include <chrono>
#include <dtCore/dtCore>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtHTransformTrajectory;
using dtCore::dtOrientationTrajectory;
using dtCore::dtPolynomialTrajectory;
using dtCore::dtPolyTrajectory;
using dtCore::dtPolyType;
using dtCore::dtTrajectory;
using dtCore::dtTrajectoryList;

void Test_PolynomialTrajectoryList() {
  double ti = 0.0, t1 = 2.0, t2 = 7.0, tf = 10.0;
  dtMath::Vector3d pi, p1, p2, pf;
  pi << 0.0, 0.0, 0.0;
  p1 << 0.5, 1.0, -0.5;
  p2 << 0.7, 0.5, 0.0;
  pf << 1.0, 1.0, 1.0;
  dtPolynomialTrajectory<double, 3> traj1(dtPolyType::CUBIC, ti, t1, pi, p1);
  dtPolynomialTrajectory<double, 3> traj2(dtPolyType::CUBIC, t1, t2, p1, p2);
  dtPolynomialTrajectory<double, 3> traj3(dtPolyType::CUBIC, t2, tf, p2, pf);
  dtTrajectoryList<dtPolynomialTrajectory<double, 3>> trajList;
  trajList.add(traj1);
  trajList.add(traj2);
  trajList.add(traj3);

  double tc;
  dtMath::Vector3d p, v, a;

  tc = 1.0;
  trajList.interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 5.0;
  trajList.interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 9.0;
  trajList.interpolate(tc, p, v, a);
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
  trajList.add(traj1);
  trajList.add(traj2);
  trajList.add(traj3);

  double tc;
  dtMath::RotationMatrix Rc;

  tc = 1.0;
  trajList.interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 5.0;
  trajList.interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 9.0;
  trajList.interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;
}

void Test_PolynomialTrajectory2() {
  double ti = 0.0;
  double tf = 10.0;
  double pi[3] = {0.0, 5.0, -10.0};
  double pf[3] = {5.0, -5.0, 0.0};
  double vi[3] = {0.0, 0.0, 0.0};
  double vf[3] = {0.0, 0.0, 0.0};
  double ai[3] = {0.0, 0.0, 0.0};
  double af[3] = {0.0, 0.0, 0.0};

  dtTrajectory<double, 3, 1> traj(tf - ti, pi, pf, vi, vf, ai, af, ti);

  double tc;
  double p[3], v[3], a[3];

  tc = 1.0;
  traj.Interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2] << ")"
            << std::endl;

  tc = 5.0;
  traj.Interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2] << ")"
            << std::endl;

  tc = 9.0;
  traj.Interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p[0] << ", " << p[1] << ", " << p[2] << ")"
            << std::endl;
}

void Test_PolynomialTrajectory() {
  double ti = 0.0;
  double tf = 10.0;
  dtMath::Vector3d pi, pf, vi, vf, ai, af;
  pi << 0.0, 5.0, -10.0;
  pf << 5.0, -5.0, 0.0;
  vi.Zero();
  vf.Zero();
  ai.Zero();
  af.Zero();
  dtPolynomialTrajectory<double, 3> traj(dtPolyType::CUBIC, ti, tf, pi, pf, vi,
                                         vf);

  double tc;
  dtMath::Vector3d p, v, a;

  tc = 1.0;
  traj.interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 5.0;
  traj.interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 9.0;
  traj.interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;
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
  traj.interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 5.0;
  traj.interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;

  tc = 9.0;
  traj.interpolate(tc, Rc);
  std::cout << "(" << tc << ", " << Rc << ")" << std::endl;
}

void TestTrajectory1dof() {
  double ti = 0.0;
  double tf = 10.0;
  dtMath::Vector3d pi;
  pi << 0.0, 0.0, 0.0;
  dtMath::Vector3d pf;
  pf << 5.0, 0.0, 0.0;

  dtPolyTrajectory<double, dtPolyType::CUBIC, 1> traj(ti, tf, pi, pf);
  double tc;
  dtMath::Vector3d p;

  tc = 1.0;
  traj.interpolate(tc, p);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 5.0;
  traj.interpolate(tc, p);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;
}

void TestTrajectoryNdof() {
  double ti = 0.0;
  double tf = 10.0;

  dtPolyTrajectory<double, dtPolyType::CUBIC, 2>::m_valueType pi, pf;
  pi << 0.0, 0.0, 0.0, 5.0, 0.0, 0.0;
  pf << 5.0, 0.0, 0.0, 0.0, 0.0, 0.0;

  dtPolyTrajectory<double, dtPolyType::CUBIC, 2> traj(ti, tf, pi, pf);

  double tc;
  dtPolyTrajectory<double, dtPolyType::CUBIC, 2>::m_valueType p;

  tc = 1.0;
  traj.interpolate(tc, p);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 5.0;
  traj.interpolate(tc, p);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;

  tc = 9.0;
  traj.interpolate(tc, p);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;
}
/*
void TestTrajectory(int sps, int loop) {
  double ti = 0.0;
  double tf = 10.0;
  dtMath::Vector3d pi;
  pi << 0.0, 0.0, 0.0;
  dtMath::Vector3d pf;
  pf << 5.0, 0.0, 0.0;
  for (int j = 0; j < loop; j++) {
    std::chrono::system_clock::time_point start =
        std::chrono::system_clock::now();
    for (int i = 0; i < sps; i++) {
      dtPolynomialTrajectory<dtMath::Vector3d, dtPolyType::CUBIC> intp(ti, tf,
                                                                       pi, pf);
      dtMath::Vector3d p;
      double t = ti + (tf - ti) / sps * i;
      intp.interpolate(t, p);
      std::cout << "(" << t << ", " << p << ")" << std::endl;
    }
    std::chrono::system_clock::time_point end =
        std::chrono::system_clock::now();
    std::chrono::duration<double> sec_elapsed = end - start;
    std::cout << "elapsed time(" << j << " th)=" << sec_elapsed.count()
              << " (sec)" << std::endl;
  }
}
*/
int main() {
  // TestTrajectory(10, 1);
  // TestTrajectory1dof();
  // TestTrajectoryNdof();
  Test_PolynomialTrajectory2();
  return 0;
}