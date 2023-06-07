#include <chrono>
#include <dtCore/dtCore>
#include <dtCore/dtTrajectory>
#include <iostream>

using dtCore::dtPolynomialTrajectory;
using dtCore::dtPolyTrajectory;
using dtCore::dtTrajType;

void Test_dtPolynomialTrajectory() {
  double t0 = 0.0;
  double tf = 10.0;
  dtMath::Vector3d pi, pf, vi, vf, ai, af;
  pi << 0.0, 5.0, -10.0;
  pf << 5.0, -5.0, 0.0;
  vi.Zero();
  vf.Zero();
  ai.Zero();
  af.Zero();
  dtPolynomialTrajectory<double, 3> traj(dtTrajType::CUBIC, t0, tf, pi, pf, vi,
                                         vf);

  double tc = 3.0;
  dtMath::Vector3d p, v, a;
  traj.interpolate(tc, p, v, a);
  std::cout << "(" << tc << ", " << p << ")" << std::endl;
}

void TestTrajectory1dof() {
  double t0 = 0.0;
  double tf = 10.0;
  dtMath::Vector3d pi;
  pi << 0.0, 0.0, 0.0;
  dtMath::Vector3d pf;
  pf << 5.0, 0.0, 0.0;

  dtPolyTrajectory<double, dtTrajType::CUBIC, 1> traj(t0, tf, pi, pf);
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
  double t0 = 0.0;
  double tf = 10.0;

  dtPolyTrajectory<double, dtTrajType::CUBIC, 2>::m_valueType pi, pf;
  pi << 0.0, 0.0, 0.0, 5.0, 0.0, 0.0;
  pf << 5.0, 0.0, 0.0, 0.0, 0.0, 0.0;

  dtPolyTrajectory<double, dtTrajType::CUBIC, 2> traj(t0, tf, pi, pf);

  double tc;
  dtPolyTrajectory<double, dtTrajType::CUBIC, 2>::m_valueType p;

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

void TestTrajectory(int sps, int loop) {
  double t0 = 0.0;
  double tf = 10.0;
  dtMath::Vector3d pi;
  pi << 0.0, 0.0, 0.0;
  dtMath::Vector3d pf;
  pf << 5.0, 0.0, 0.0;
  for (int j = 0; j < loop; j++) {
    std::chrono::system_clock::time_point start =
        std::chrono::system_clock::now();
    for (int i = 0; i < sps; i++) {
      dtPolynomialTrajectory<dtMath::Vector3d, dtTrajType::CUBIC> intp(t0, tf,
                                                                       pi, pf);
      dtMath::Vector3d p;
      double t = t0 + (tf - t0) / sps * i;
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

int main() {
  // TestTrajectory(10, 1);
  TestTrajectory1dof();
  TestTrajectoryNdof();
  return 0;
}