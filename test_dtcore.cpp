#include "dtCore/dtInterpolator.h"
#include <chrono>
#include <iostream>

void TestInterpolator(dtCore::dtInterpolator::TYPE type, int sps, int loop) {
  double t0 = 0.0;
  double tf = 10.0;
  dtMath::Vector3d vi;
  vi << 0.0, 0.0, 0.0;
  dtMath::Vector3d vf;
  vf << 5.0, 0.0, 0.0;
  for (int j = 0; j < loop; j++) {
    std::chrono::system_clock::time_point start =
        std::chrono::system_clock::now();
    for (int i = 0; i < sps; i++) {
      dtCore::dtInterpolator intp(type, t0, tf, vi, vf);
      double p, v, a;
      double t = t0 + (tf - t0) / sps * i;
      intp.interpolate(t, p, v, a);
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
  TestInterpolator(dtCore::dtInterpolator::TYPE::CUBIC, 1000, 1);
  return 0;
}