#include <iomanip>
#include <sstream>
#include <string>

#include <benchmark/benchmark.h>

#include "geoarrow/geoarrow.h"

namespace {

std::string WKTInput(int kind) {
  if (kind == 0) {
    return "POINT(-81.47275543212891 36.23435592651367)";
  }
  if (kind == 1) {
    return "POLYGON((35 10,45 45,15 40,10 20,35 10),"
           "(20 30,35 35,30 20,20 30))";
  }

  std::ostringstream out;
  out << std::setprecision(17) << "LINESTRING(";
  uint64_t state = 161;
  for (int i = 0; i < 10000; i++) {
    if (i != 0) out << ',';
    for (int j = 0; j < 2; j++) {
      if (j != 0) out << ' ';
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      if (kind == 2 || kind == 4) {
        int value = static_cast<int>(state % 100);
        out << ((kind == 4 && (state >> 32) % 2) ? -value : value);
      } else {
        double unit = static_cast<double>(state >> 11) / (uint64_t{1} << 53);
        out << (unit * 360.0 - 180.0);
      }
    }
  }
  out << ')';
  return out.str();
}

void WKTReaderVisit(benchmark::State& state, int kind) {
  const std::string input = WKTInput(kind);
  const GeoArrowStringView view{input.data(), static_cast<int64_t>(input.size())};
  GeoArrowWKTReader reader;
  if (GeoArrowWKTReaderInit(&reader) != GEOARROW_OK) {
    state.SkipWithError("Failed to initialize WKT reader");
    return;
  }
  GeoArrowVisitor visitor;
  GeoArrowVisitorInitVoid(&visitor);
  GeoArrowError error;
  visitor.error = &error;

  for (auto _ : state) {
    int result = GeoArrowWKTReaderVisit(&reader, view, &visitor);
    benchmark::DoNotOptimize(result);
    if (result != GEOARROW_OK) {
      state.SkipWithError(error.message);
      break;
    }
  }
  state.SetBytesProcessed(state.iterations() * view.size_bytes);
  GeoArrowWKTReaderReset(&reader);
}

BENCHMARK_CAPTURE(WKTReaderVisit, Point, 0);
BENCHMARK_CAPTURE(WKTReaderVisit, ShortIntegers, 1);
BENCHMARK_CAPTURE(WKTReaderVisit, IntegerLine, 2);
BENCHMARK_CAPTURE(WKTReaderVisit, SignedIntegerLine, 4);
BENCHMARK_CAPTURE(WKTReaderVisit, DecimalLine, 3);

}  // namespace
