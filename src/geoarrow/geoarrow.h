
#ifndef GEOARROW_H_INCLUDED
#define GEOARROW_H_INCLUDED

#include <stdint.h>

#include "geoarrow_type.h"

#ifdef __cplusplus
extern "C" {
#endif

GeoArrowErrorCode GeoArrowSchemaInit(struct ArrowSchema* schema, enum GeoArrowType type);

GeoArrowErrorCode GeoArrowSchemaInitExtension(struct ArrowSchema* schema,
                                              enum GeoArrowType type);

GeoArrowErrorCode GeoArrowSchemaViewInit(struct GeoArrowSchemaView* schema_view,
                                         struct ArrowSchema* schema,
                                         struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowSchemaViewInitFromStorage(
    struct GeoArrowSchemaView* schema_view, struct ArrowSchema* schema,
    struct GeoArrowStringView extension_name, struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowSchemaViewInitFromType(struct GeoArrowSchemaView* schema_view,
                                                 enum GeoArrowType type);

GeoArrowErrorCode GeoArrowMetadataViewInit(struct GeoArrowMetadataView* metadata_view,
                                           struct GeoArrowStringView metadata,
                                           struct GeoArrowError* error);

int64_t GeoArrowMetadataSerialize(const struct GeoArrowMetadataView* metadata_view,
                                  char* out, int64_t n);

GeoArrowErrorCode GeoArrowSchemaSetMetadata(
    struct ArrowSchema* schema, const struct GeoArrowMetadataView* metadata_view);

GeoArrowErrorCode GeoArrowSchemaSetMetadataDeprecated(
    struct ArrowSchema* schema, const struct GeoArrowMetadataView* metadata_view);

GeoArrowErrorCode GeoArrowSchemaSetMetadataFrom(struct ArrowSchema* schema,
                                                struct ArrowSchema* schema_src);

int64_t GeoArrowUnescapeCrs(struct GeoArrowStringView crs, char* out, int64_t n);

GeoArrowErrorCode GeoArrowArrayViewInitFromType(struct GeoArrowArrayView* array_view,
                                                enum GeoArrowType type);

GeoArrowErrorCode GeoArrowArrayViewInitFromSchema(struct GeoArrowArrayView* array_view,
                                                  struct ArrowSchema* schema,
                                                  struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowArrayViewSetArray(struct GeoArrowArrayView* array_view,
                                            struct ArrowArray* array,
                                            struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowArrayViewVisit(struct GeoArrowArrayView* array_view,
                                         int64_t offset, int64_t length,
                                         struct GeoArrowVisitor* v);

void GeoArrowVisitorInitVoid(struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowKernelInit(struct GeoArrowKernel* kernel, const char* name,
                                     const char* options);

struct GeoArrowWKTWriter {
  int significant_digits;
  int use_flat_multipoint;
  int64_t max_element_size_bytes;
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKTWriterInit(struct GeoArrowWKTWriter* writer);

void GeoArrowWKTWriterInitVisitor(struct GeoArrowWKTWriter* writer,
                                  struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowWKTWriterFinish(struct GeoArrowWKTWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error);

void GeoArrowWKTWriterReset(struct GeoArrowWKTWriter* writer);

struct GeoArrowWKTReader {
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKTReaderInit(struct GeoArrowWKTReader* reader);

GeoArrowErrorCode GeoArrowWKTReaderVisit(struct GeoArrowWKTReader* reader,
                                         struct GeoArrowStringView s,
                                         struct GeoArrowVisitor* v);

void GeoArrowWKTReaderReset(struct GeoArrowWKTReader* reader);

struct GeoArrowWKBWriter {
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKBWriterInit(struct GeoArrowWKBWriter* writer);

void GeoArrowWKBWriterInitVisitor(struct GeoArrowWKBWriter* writer,
                                  struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowWKBWriterFinish(struct GeoArrowWKBWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error);

void GeoArrowWKBWriterReset(struct GeoArrowWKBWriter* writer);

struct GeoArrowWKBReader {
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKBReaderInit(struct GeoArrowWKBReader* reader);

void GeoArrowWKBReaderReset(struct GeoArrowWKBReader* reader);

GeoArrowErrorCode GeoArrowWKBReaderVisit(struct GeoArrowWKBReader* reader,
                                         struct GeoArrowBufferView src,
                                         struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowBuilderInitFromType(struct GeoArrowBuilder* builder,
                                              enum GeoArrowType type);

GeoArrowErrorCode GeoArrowBuilderInitFromSchema(struct GeoArrowBuilder* builder,
                                                struct ArrowSchema* schema,
                                                struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowBuilderReserveBuffer(struct GeoArrowBuilder* builder, int64_t i,
                                               int64_t additional_size_bytes);

static inline int GeoArrowBuilderBufferCheck(struct GeoArrowBuilder* builder, int64_t i,
                                             int64_t additional_size_bytes);

static inline void GeoArrowBuilderAppendBufferUnsafe(struct GeoArrowBuilder* builder,
                                                     int64_t i,
                                                     struct GeoArrowBufferView value);

static inline GeoArrowErrorCode GeoArrowBuilderAppendBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value);

GeoArrowErrorCode GeoArrowBuilderSetOwnedBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value,
    void (*custom_free)(uint8_t* ptr, int64_t size, void* private_data),
    void* private_data);

GeoArrowErrorCode GeoArrowBuilderInitVisitor(struct GeoArrowBuilder* builder,
                                             struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowBuilderFinish(struct GeoArrowBuilder* builder,
                                        struct ArrowArray* array,
                                        struct GeoArrowError* error);

void GeoArrowBuilderReset(struct GeoArrowBuilder* builder);

#ifdef __cplusplus
}
#endif

#include "geoarrow_type_inline.h"

// These functions have to live here because they are inline but use non-inline
// functions too.

static inline GeoArrowErrorCode GeoArrowBuilderAppendBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value) {
  if (!GeoArrowBuilderBufferCheck(builder, i, value.size_bytes)) {
    int result = GeoArrowBuilderReserveBuffer(builder, i, value.size_bytes);
    if (result != GEOARROW_OK) {
      return result;
    }
  }

  GeoArrowBuilderAppendBufferUnsafe(builder, i, value);
  return GEOARROW_OK;
}

static inline GeoArrowErrorCode GeoArrowBuilderCoordsReserve(
    struct GeoArrowBuilder* builder, int64_t additional_size_coords) {
  if (GeoArrowBuilderCoordsCheck(builder, additional_size_coords)) {
    return GEOARROW_OK;
  }

  struct GeoArrowWritableCoordView* writable_view = &builder->view.coords;
  int result;
  int64_t last_buffer = builder->view.n_buffers - 1;
  int n_values = writable_view->n_values;

  switch (builder->view.schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      // Sync the coord view size back to the buffer size
      builder->view.buffers[last_buffer].size_bytes =
          writable_view->size_coords * sizeof(double) * n_values;

      // Use the normal reserve
      result = GeoArrowBuilderReserveBuffer(
          builder, last_buffer, additional_size_coords * sizeof(double) * n_values);
      if (result != GEOARROW_OK) {
        return result;
      }

      // Sync the capacity and pointers back to the writable view
      writable_view->capacity_coords =
          builder->view.buffers[last_buffer].capacity_bytes / sizeof(double) / n_values;
      for (int i = 0; i < n_values; i++) {
        writable_view->values[i] = builder->view.buffers[last_buffer].data.as_double + i;
      }

      return GEOARROW_OK;

    case GEOARROW_COORD_TYPE_SEPARATE:
      for (int64_t i = last_buffer - n_values + 1; i <= last_buffer; i++) {
        // Sync the coord view size back to the buffer size
        builder->view.buffers[i].size_bytes = writable_view->size_coords * sizeof(double);

        // Use the normal reserve
        result = GeoArrowBuilderReserveBuffer(builder, i,
                                              additional_size_coords * sizeof(double));
        if (result != GEOARROW_OK) {
          return result;
        }
      }

      // Sync the capacity and pointers back to the writable view
      writable_view->capacity_coords =
          builder->view.buffers[last_buffer].capacity_bytes / sizeof(double);
      for (int i = 0; i < n_values; i++) {
        writable_view->values[i] =
            builder->view.buffers[last_buffer - n_values + 1 + i].data.as_double;
      }

      return GEOARROW_OK;
    default:
      // Beacuse there is no include <errno.h> here yet
      return -1;
  }
}

static inline GeoArrowErrorCode GeoArrowBuilderCoordsAppend(
    struct GeoArrowBuilder* builder, const struct GeoArrowCoordView* coords,
    enum GeoArrowDimensions dimensions, int64_t offset, int64_t n) {
  if (!GeoArrowBuilderCoordsCheck(builder, n)) {
    int result = GeoArrowBuilderCoordsReserve(builder, n);
    if (result != GEOARROW_OK) {
      return result;
    }
  }

  GeoArrowBuilderCoordsAppendUnsafe(builder, coords, dimensions, offset, n);
  return GEOARROW_OK;
}

static inline GeoArrowErrorCode GeoArrowBuilderOffsetReserve(
    struct GeoArrowBuilder* builder, int32_t i, int64_t additional_size_elements) {
  if (GeoArrowBuilderOffsetCheck(builder, i, additional_size_elements)) {
    return GEOARROW_OK;
  }

  return GeoArrowBuilderReserveBuffer(builder, i + 1,
                                      additional_size_elements * sizeof(int32_t));
}

static inline GeoArrowErrorCode GeoArrowBuilderOffsetAppend(
    struct GeoArrowBuilder* builder, int32_t i, int32_t* data,
    int64_t additional_size_elements) {
  if (!GeoArrowBuilderOffsetCheck(builder, i, additional_size_elements)) {
    int result = GeoArrowBuilderOffsetReserve(builder, i, additional_size_elements);
    if (result != GEOARROW_OK) {
      return result;
    }
  }

  GeoArrowBuilderOffsetAppendUnsafe(builder, i, data, additional_size_elements);
  return GEOARROW_OK;
}

#endif
