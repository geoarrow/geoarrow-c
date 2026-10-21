
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "geoarrow/geoarrow.h"

namespace {

// Use several features so each scalar lookup advances through coordinate buffers.
// All allocation and fixture construction happens outside the timed loop.
struct GeometryFixture {
  static constexpr int64_t kNumFeatures = 64;
  struct GeoArrowArrayView view;
  std::vector<int32_t> offsets[3];
  std::vector<double> coords;

  GeometryFixture(enum GeoArrowType type, int32_t sequence_size) {
    if (GeoArrowArrayViewInitFromType(&view, type) != GEOARROW_OK) {
      throw std::runtime_error("Failed to initialize array view");
    }

    int64_t size = kNumFeatures;
    for (int level = 0; level < view.n_offsets; level++) {
      view.length[level] = size;
      // Four rings/parts per container; sequence_size coordinates per sequence.
      int32_t child_size = level == view.n_offsets - 1 ? sequence_size : 4;
      offsets[level].resize(size + 1);
      for (int64_t i = 0; i <= size; i++) {
        offsets[level][i] = static_cast<int32_t>(i * child_size);
      }
      view.offsets[level] = offsets[level].data();
      view.first_offset[level] = 0;
      view.last_offset[level] = offsets[level].back();
      size *= child_size;
    }

    view.length[view.n_offsets] = size;
    view.coords.n_coords = size;
    coords.resize(size * view.coords.n_values);
    for (int j = 0; j < view.coords.n_values; j++) {
      int64_t offset =
          view.schema_view.coord_type == GEOARROW_COORD_TYPE_SEPARATE ? j * size : j;
      view.coords.values[j] = coords.data() + offset;
      for (int64_t i = 0; i < size; i++) {
        coords[offset + i * view.coords.coords_stride] = static_cast<double>(i + j);
      }
    }
  }
};

struct Geometry {
  struct GeoArrowGeometry value;
  Geometry() {
    if (GeoArrowGeometryInit(&value) != GEOARROW_OK) {
      throw std::runtime_error("Failed to initialize geometry");
    }
  }
  ~Geometry() { GeoArrowGeometryReset(&value); }
};

// A native-array-only visitor that borrows coordinates. This baseline isolates
// visitor callback/node-building overhead from the coordinate copying performed
// by GeoArrowGeometryInitVisitor(). Native arrays emit one coords event per leaf.
struct BorrowingVisitor {
  Geometry geometry;
  struct GeoArrowVisitor visitor;
  int level = 0;
  int64_t parents[3];

  BorrowingVisitor() {
    GeoArrowVisitorInitVoid(&visitor);
    visitor.private_data = this;
    visitor.feat_start = &FeatureStart;
    visitor.null_feat = &NullFeature;
    visitor.geom_start = &GeometryStart;
    visitor.ring_start = &RingStart;
    visitor.coords = &Coords;
    visitor.geom_end = &GeometryEnd;
    visitor.ring_end = &GeometryEnd;
  }

  static BorrowingVisitor& Self(struct GeoArrowVisitor* v) {
    return *static_cast<BorrowingVisitor*>(v->private_data);
  }

  static int FeatureStart(struct GeoArrowVisitor* v) {
    auto& self = Self(v);
    self.level = 0;
    return GeoArrowGeometryResizeNodesInline(&self.geometry.value, 0);
  }

  static int NullFeature(struct GeoArrowVisitor* v) {
    struct GeoArrowGeometryNode* node;
    return GeoArrowGeometryAppendNodeInline(&Self(v).geometry.value, &node);
  }

  static int GeometryStart(struct GeoArrowVisitor* v, enum GeoArrowGeometryType type,
                           enum GeoArrowDimensions dimensions) {
    auto& self = Self(v);
    auto& geom = self.geometry.value;
    struct GeoArrowGeometryNode* node;
    GEOARROW_RETURN_NOT_OK(GeoArrowGeometryAppendNodeInline(&geom, &node));
    node->geometry_type = static_cast<uint8_t>(type);
    node->dimensions = static_cast<uint8_t>(dimensions);
    node->level = static_cast<uint8_t>(self.level);
    if (self.level > 0) {
      geom.root[self.parents[self.level - 1]].size++;
    }
    self.parents[self.level++] = geom.size_nodes - 1;
    return GEOARROW_OK;
  }

  static int RingStart(struct GeoArrowVisitor* v) {
    return GeometryStart(
        v, GEOARROW_GEOMETRY_TYPE_LINESTRING,
        static_cast<GeoArrowDimensions>(Self(v).geometry.value.root->dimensions));
  }

  static int Coords(struct GeoArrowVisitor* v, const struct GeoArrowCoordView* coords) {
    auto& geom = Self(v).geometry.value;
    auto& node = geom.root[geom.size_nodes - 1];
    node.size = static_cast<uint32_t>(coords->n_coords);
    if (node.size > 0) {
      for (int j = 0; j < coords->n_values; j++) {
        node.coords[j] = reinterpret_cast<const uint8_t*>(coords->values[j]);
        node.coord_stride[j] = coords->coords_stride * sizeof(double);
      }
    }
    return GEOARROW_OK;
  }

  static int GeometryEnd(struct GeoArrowVisitor* v) {
    Self(v).level--;
    return GEOARROW_OK;
  }
};

// Check actual node structure and coordinates before timing either path. The
// visitor owns copied coordinates whereas GetGeometry borrows the array buffers.
void CheckEqual(const struct GeoArrowGeometry& lhs, const struct GeoArrowGeometry& rhs) {
  if (lhs.size_nodes != rhs.size_nodes) {
    throw std::runtime_error("Geometry node counts differ");
  }
  for (int64_t i = 0; i < lhs.size_nodes; i++) {
    const auto& a = lhs.root[i];
    const auto& b = rhs.root[i];
    if (a.geometry_type != b.geometry_type || a.dimensions != b.dimensions ||
        a.level != b.level || a.size != b.size) {
      throw std::runtime_error("Geometry nodes differ");
    }
    if (a.geometry_type != GEOARROW_GEOMETRY_TYPE_POINT &&
        a.geometry_type != GEOARROW_GEOMETRY_TYPE_LINESTRING) {
      continue;
    }
    for (uint32_t k = 0; k < a.size; k++) {
      for (int j = 0; j < _GeoArrowkNumDimensions[a.dimensions]; j++) {
        if (std::memcmp(a.coords[j] + k * a.coord_stride[j],
                        b.coords[j] + k * b.coord_stride[j], sizeof(double)) != 0) {
          throw std::runtime_error("Geometry coordinates differ");
        }
      }
    }
  }
}

enum class Strategy { Direct, CopyingVisitor, BorrowingVisitor };

template <Strategy strategy>
void ArrayViewGeometry(benchmark::State& state, enum GeoArrowType type,
                       int32_t sequence_size) {
  GeometryFixture fixture(type, sequence_size);
  Geometry direct;
  Geometry copied;
  BorrowingVisitor borrowed;
  struct GeoArrowVisitor visitor;
  GeoArrowGeometryInitVisitor(&copied.value, &visitor);
  visitor.error = nullptr;

  // Warm all reusable outputs, then validate the first and last feature.
  for (int64_t i : {int64_t{0}, GeometryFixture::kNumFeatures - 1}) {
    if (GeoArrowArrayViewGetGeometry(&fixture.view, i, &direct.value) != GEOARROW_OK ||
        GeoArrowArrayViewVisitNative(&fixture.view, i, 1, &visitor) != GEOARROW_OK ||
        GeoArrowArrayViewVisitNative(&fixture.view, i, 1, &borrowed.visitor) !=
            GEOARROW_OK) {
      state.SkipWithError("Failed to read geometry");
      return;
    }
    CheckEqual(direct.value, copied.value);
    CheckEqual(direct.value, borrowed.geometry.value);
  }

  auto& result = strategy == Strategy::Direct           ? direct.value
                 : strategy == Strategy::CopyingVisitor ? copied.value
                                                        : borrowed.geometry.value;
  for (auto _ : state) {
    for (int64_t i = 0; i < GeometryFixture::kNumFeatures; i++) {
      int code;
      if constexpr (strategy == Strategy::CopyingVisitor) {
        code = GeoArrowArrayViewVisitNative(&fixture.view, i, 1, &visitor);
      } else if constexpr (strategy == Strategy::BorrowingVisitor) {
        code = GeoArrowArrayViewVisitNative(&fixture.view, i, 1, &borrowed.visitor);
      } else {
        code = GeoArrowArrayViewGetGeometry(&fixture.view, i, &direct.value);
      }
      if (code != GEOARROW_OK) {
        state.SkipWithError("Failed to read geometry");
        return;
      }
      benchmark::DoNotOptimize(result.root);
      benchmark::ClobberMemory();
    }
  }
  state.SetItemsProcessed(state.iterations() * GeometryFixture::kNumFeatures);
}

bool RegisterArrayViewGeometryBenchmarks() {
  const char* dimensions[] = {"", "XY", "XYZ", "XYM", "XYZM"};
  for (int geometry_type = 1; geometry_type <= 6; geometry_type++) {
    for (int dims = 1; dims <= 4; dims++) {
      for (auto layout :
           {GEOARROW_COORD_TYPE_SEPARATE, GEOARROW_COORD_TYPE_INTERLEAVED}) {
        for (int32_t sequence_size : {4, 1024}) {
          if (geometry_type == GEOARROW_GEOMETRY_TYPE_POINT && sequence_size != 4) {
            continue;
          }
          auto type = GeoArrowMakeType(static_cast<GeoArrowGeometryType>(geometry_type),
                                       static_cast<GeoArrowDimensions>(dims), layout);
          std::string suffix =
              std::string("/") +
              GeoArrowGeometryTypeString(
                  static_cast<GeoArrowGeometryType>(geometry_type)) +
              "/" + dimensions[dims] + "/" +
              (layout == GEOARROW_COORD_TYPE_SEPARATE ? "Separate" : "Interleaved") +
              "/" +
              std::to_string(
                  geometry_type == GEOARROW_GEOMETRY_TYPE_POINT ? 1 : sequence_size);
          benchmark::RegisterBenchmark(("GetGeometry" + suffix).c_str(),
                                       &ArrayViewGeometry<Strategy::Direct>, type,
                                       sequence_size);
          benchmark::RegisterBenchmark(("Visitor" + suffix).c_str(),
                                       &ArrayViewGeometry<Strategy::CopyingVisitor>, type,
                                       sequence_size);
          benchmark::RegisterBenchmark(("BorrowingVisitor" + suffix).c_str(),
                                       &ArrayViewGeometry<Strategy::BorrowingVisitor>,
                                       type, sequence_size);
        }
      }
    }
  }
  return true;
}

const bool kRegistered = RegisterArrayViewGeometryBenchmarks();

}  // namespace
