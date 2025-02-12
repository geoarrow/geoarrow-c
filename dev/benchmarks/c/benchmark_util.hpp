
#include <cmath>
#include <cstdlib>

namespace geoarrow {

namespace benchmark_util {

static const int64_t kNumCoordsPrettyBig = 1000000;

static inline void PointsOnCircle(uint32_t n, uint32_t stride, double* out_x,
                                  double* out_y, double dangle_radians = M_PI / 100.0,
                                  double radius = 483.0) {
  double angle = 0;

  for (uint32_t i = 0; i < n; i++) {
    *out_x = std::cos(angle) * radius;
    *out_y = std::sin(angle) * radius;
    angle += dangle_radians;
    out_x += stride;
    out_y += stride;
  }
}

static inline void FillRandom(uint32_t n, uint32_t stride, double* out,
                              uint32_t seed = 1234, double range_min = -1.0,
                              double range_max = 1.0) {
  std::srand(seed);
  double range = range_max - range_min;
  for (uint32_t i = 0; i < n; i++) {
    double value01 = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    *out = range_min + value01 * range;
    out += stride;
  }
}

}  // namespace benchmark_util
}  // namespace geoarrow
