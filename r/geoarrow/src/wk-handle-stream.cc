
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include <vector>

#include "geoarrow.h"
#include "wk-v1.h"

// #define HANDLE_CONTINUE_OR_BREAK(expr)                         \
//     result = expr;                                             \
//     if (result == geoarrow::Handler::Result::ABORT_FEATURE) \
//         continue; \
//     else if (result == geoarrow::Handler::Result::ABORT) break

class WKGeoArrowHandler {
 public:
  WKGeoArrowHandler(wk_handler_t* handler, R_xlen_t size)
      : handler_(handler), feat_id_(-1), ring_id_(-1), coord_id_(-1) {
    WK_VECTOR_META_RESET(vector_meta_, WK_GEOMETRY);
    WK_META_RESET(meta_, WK_GEOMETRY);

    vector_meta_.size = size;

    // This is to keep vectors from being reallocated, since some
    // wk handlers assume that the meta pointers will stay valid between
    // the start and end geometry methods (this will get fixed in a
    // wk release soon)
    part_id_stack_.reserve(32);
    meta_stack_.reserve(32);
  }

  // Visitor interface
  void InitVisitor(struct GeoArrowVisitor* v) {
    v->feat_start = &feat_start_visitor;
    v->null_feat = &null_feat_visitor;
    v->geom_start = &geom_start_visitor;
    v->ring_start = &ring_start_visitor;
    v->coords = &coords_visitor;
    v->ring_end = &ring_end_visitor;
    v->geom_end = &geom_end_visitor;
    v->feat_end = &feat_end_visitor;
    v->private_data = this;
  }

  void set_vector_geometry_type(GeoArrowGeometryType geometry_type) {
    vector_meta_.geometry_type = geometry_type;
  }

  void set_vector_dimensions(GeoArrowDimensions dimensions) {
    vector_meta_.flags &= ~WK_FLAG_HAS_Z;
    vector_meta_.flags &= ~WK_FLAG_HAS_M;

    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XYZ:
      case GEOARROW_DIMENSIONS_XYZM:
        vector_meta_.flags |= WK_FLAG_HAS_Z;
        break;
      default:
        break;
    }

    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XYM:
      case GEOARROW_DIMENSIONS_XYZM:
        vector_meta_.flags |= WK_FLAG_HAS_M;
        break;
      default:
        break;
    }

    if (dimensions == GEOARROW_DIMENSIONS_UNKNOWN) {
      vector_meta_.flags |= WK_FLAG_DIMS_UNKNOWN;
    } else {
      vector_meta_.flags &= ~WK_FLAG_DIMS_UNKNOWN;
    }
  }

  void set_meta_dimensions(GeoArrowDimensions dimensions) {
    meta_.flags &= ~WK_FLAG_HAS_Z;
    meta_.flags &= ~WK_FLAG_HAS_M;

    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XYZ:
      case GEOARROW_DIMENSIONS_XYZM:
        meta_.flags |= WK_FLAG_HAS_Z;
        break;
      default:
        break;
    }

    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XYM:
      case GEOARROW_DIMENSIONS_XYZM:
        meta_.flags |= WK_FLAG_HAS_M;
        break;
      default:
        break;
    }
  }

  int feat_start() {
    feat_id_++;
    part_id_stack_.clear();
    meta_stack_.clear();
    return handler_->feature_start(&vector_meta_, feat_id_, handler_->handler_data);
  }

  int null_feat() { return handler_->null_feature(handler_->handler_data); }

  int geom_start(GeoArrowGeometryType geometry_type, GeoArrowDimensions dimensions,
                 uint32_t size) {
    ring_id_ = -1;
    coord_id_ = -1;

    if (part_id_stack_.size() > 0) {
      part_id_stack_[part_id_stack_.size() - 1]++;
    }

    meta_.geometry_type = geometry_type;
    meta_.size = size;
    set_meta_dimensions(dimensions);
    meta_stack_.push_back(meta_);

    int result = handler_->geometry_start(meta(), part_id(), handler_->handler_data);
    part_id_stack_.push_back(-1);
    return result;
  }

  int ring_start(uint32_t size) {
    ring_id_++;
    coord_id_ = -1;
    ring_size_ = size;
    return handler_->ring_start(meta(), ring_size_, ring_id_, handler_->handler_data);
  }

  int coords(const struct GeoArrowCoordView* coords) {
    int result;
    double coord[4];
    for (int64_t i = 0; i < coords->n_coords; i++) {
      coord_id_++;
      for (int j = 0; j < coords->n_values; j++) {
        coord[j] = GEOARROW_COORD_VIEW_VALUE(coords, i, j);
      }

      result = handler_->coord(meta(), coord, coord_id_, handler_->handler_data);
      if (result != WK_CONTINUE) {
        return result;
      }
    }

    return WK_CONTINUE;
  }

  int ring_end() {
    return handler_->ring_end(meta(), ring_size_, ring_id_, handler_->handler_data);
  }

  int geom_end() {
    if (part_id_stack_.size() > 0) part_id_stack_.pop_back();
    int result = handler_->geometry_end(meta(), part_id(), handler_->handler_data);
    if (meta_stack_.size() > 0) meta_stack_.pop_back();
    return (int)result;
  }

  int feat_end() {
    return handler_->feature_end(&vector_meta_, feat_id_, handler_->handler_data);
  }

  wk_vector_meta_t vector_meta_;

 private:
  wk_handler_t* handler_;

  std::vector<wk_meta_t> meta_stack_;
  std::vector<int32_t> part_id_stack_;
  wk_meta_t meta_;

  int32_t ring_size_;
  int64_t feat_id_;

  int32_t ring_id_;
  int32_t coord_id_;

  int32_t part_id() {
    if (part_id_stack_.size() == 0) {
      return WK_PART_ID_NONE;
    } else {
      return part_id_stack_[part_id_stack_.size() - 1];
    }
  }

  const wk_meta_t* meta() {
    if (meta_stack_.size() == 0) {
      throw std::runtime_error("geom_start()/geom_end() stack imbalance <meta>");
    }
    return meta_stack_.data() + meta_stack_.size() - 1;
  }

  static int wrap_result(int result, GeoArrowError* error) {
    if (result == WK_ABORT_FEATURE) {
      GeoArrowErrorSet(error, "WK_ABORT_FEATURE");
      return EALREADY;
    }

    if (result != WK_CONTINUE) {
      GeoArrowErrorSet(error, "result !+ WK_CONTINUE (%d)", result);
      return EINVAL;
    } else {
      return GEOARROW_OK;
    }
  }

  static int feat_start_visitor(struct GeoArrowVisitor* v) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->feat_start();
    return wrap_result(result, v->error);
  }

  static int null_feat_visitor(struct GeoArrowVisitor* v) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->null_feat();
    return wrap_result(result, v->error);
  }

  static int geom_start_visitor(struct GeoArrowVisitor* v,
                                enum GeoArrowGeometryType geometry_type,
                                enum GeoArrowDimensions dimensions) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->geom_start(geometry_type, dimensions, WK_SIZE_UNKNOWN);
    return wrap_result(result, v->error);
  }

  static int ring_start_visitor(struct GeoArrowVisitor* v) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->ring_start(WK_SIZE_UNKNOWN);
    return wrap_result(result, v->error);
  }

  static int coords_visitor(struct GeoArrowVisitor* v,
                            const struct GeoArrowCoordView* coords) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->coords(coords);
    return wrap_result(result, v->error);
  }

  static int ring_end_visitor(struct GeoArrowVisitor* v) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->ring_end();
    return wrap_result(result, v->error);
  }

  static int geom_end_visitor(struct GeoArrowVisitor* v) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->geom_end();
    return wrap_result(result, v->error);
  }

  static int feat_end_visitor(struct GeoArrowVisitor* v) {
    auto private_data = reinterpret_cast<WKGeoArrowHandler*>(v->private_data);
    int result = private_data->feat_end();
    return wrap_result(result, v->error);
  }
};

SEXP geoarrow_handle_stream(SEXP data, wk_handler_t* handler) {
  //     CPP_START

  //     struct ArrowArrayStream* array_stream = array_stream_from_xptr(VECTOR_ELT(data,
  //     0), "handleable"); struct ArrowSchema* schema = schema_from_xptr(VECTOR_ELT(data,
  //     1), "schema"); SEXP n_features_sexp = VECTOR_ELT(data, 2);

  //     // We can't stack allocate this because we don't know the exact type that is
  //     returned
  //     // and we can't rely on the deleter to run because one of the handler
  //     // calls could longjmp. We use the same trick for making sure the array_data is
  //     // released for each array in the stream.
  //     geoarrow::ArrayView* view = geoarrow::create_view(schema);
  //     SEXP view_xptr = PROTECT(R_MakeExternalPtr(view, R_NilValue, R_NilValue));
  //     R_RegisterCFinalizer(view_xptr, &delete_array_view_xptr);

  //     R_xlen_t vector_size = WK_VECTOR_SIZE_UNKNOWN;
  //     if (TYPEOF(n_features_sexp) == INTSXP) {
  //         if (INTEGER(n_features_sexp)[0] != NA_INTEGER) {
  //             vector_size = INTEGER(n_features_sexp)[0];
  //         }
  //     } else {
  //         double n_features_double = REAL(n_features_sexp)[0];
  //         if (!ISNA(n_features_double) && !ISNAN(n_features_double)) {
  //             vector_size = n_features_double;
  //         }
  //     }

  //     WKGeoArrowHandler geoarrow_handler(handler, vector_size);
  //     view->read_meta(&geoarrow_handler);

  //     int result = handler->vector_start(&geoarrow_handler.vector_meta_,
  //     handler->handler_data); if (result == WK_CONTINUE) {
  //         struct ArrowArray* array_data = (struct ArrowArray*) malloc(sizeof(struct
  //         ArrowArray)); if (array_data == NULL) {
  //             Rf_error("Failed to allocate struct ArrowArray");
  //         }
  //         array_data->release = NULL;
  //         SEXP array_data_wrapper = PROTECT(R_MakeExternalPtr(array_data, R_NilValue,
  //         R_NilValue)); R_RegisterCFinalizer(array_data_wrapper,
  //         &geoarrow_finalize_array_data);

  //         int stream_result = 0;
  //         while(result != WK_ABORT) {
  //             if (array_data->release != NULL) {
  //                 array_data->release(array_data);
  //             }
  //             stream_result = array_stream->get_next(array_stream, array_data);
  //             if (stream_result != 0) {
  //                 const char* error_message =
  //                 array_stream->get_last_error(array_stream); if (error_message !=
  //                 NULL) {
  //                     Rf_error("[%d] %s", stream_result, error_message);
  //                 } else {
  //                     Rf_error("ArrowArrayStream->get_next() failed with code %d",
  //                     stream_result);
  //                 }
  //             }

  //             if (array_data->release == NULL) {
  //                 break;
  //             }

  //             view->set_array(array_data);
  //             result = (int) view->read_features(&geoarrow_handler);
  //             if (result == WK_CONTINUE) {
  //                 continue;
  //             } else if (result == WK_ABORT) {
  //                 break;
  //             }
  //         }

  //         UNPROTECT(1);
  //     }

  //     SEXP result_sexp = PROTECT(handler->vector_end(&geoarrow_handler.vector_meta_,
  //     handler->handler_data)); UNPROTECT(2); return result_sexp;

  //     CPP_END
  return R_NilValue;
}

extern "C" SEXP geoarrow_c_handle_stream(SEXP data, SEXP handler_xptr) {
  return wk_handler_run_xptr(&geoarrow_handle_stream, data, handler_xptr);
}
