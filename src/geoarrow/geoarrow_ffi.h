
#ifndef GEOARROW_FFI_INCLUDED
#define GEOARROW_FFI_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Extra guard for versions of Arrow without the canonical guard
#ifndef ARROW_FLAG_DICTIONARY_ORDERED

#ifndef ARROW_C_DATA_INTERFACE
#define ARROW_C_DATA_INTERFACE

#define ARROW_FLAG_DICTIONARY_ORDERED 1
#define ARROW_FLAG_NULLABLE 2
#define ARROW_FLAG_MAP_KEYS_SORTED 4

struct ArrowSchema {
  // Array type description
  const char* format;
  const char* name;
  const char* metadata;
  int64_t flags;
  int64_t n_children;
  struct ArrowSchema** children;
  struct ArrowSchema* dictionary;

  // Release callback
  void (*release)(struct ArrowSchema*);
  // Opaque producer-specific data
  void* private_data;
};

struct ArrowArray {
  // Array data description
  int64_t length;
  int64_t null_count;
  int64_t offset;
  int64_t n_buffers;
  int64_t n_children;
  const void** buffers;
  struct ArrowArray** children;
  struct ArrowArray* dictionary;

  // Release callback
  void (*release)(struct ArrowArray*);
  // Opaque producer-specific data
  void* private_data;
};

#endif  // ARROW_C_DATA_INTERFACE

#ifndef ARROW_C_STREAM_INTERFACE
#define ARROW_C_STREAM_INTERFACE

struct ArrowArrayStream {
  // Callback to get the stream type
  // (will be the same for all arrays in the stream).
  //
  // Return value: 0 if successful, an `errno`-compatible error code otherwise.
  //
  // If successful, the ArrowSchema must be released independently from the stream.
  int (*get_schema)(struct ArrowArrayStream*, struct ArrowSchema* out);

  // Callback to get the next array
  // (if no error and the array is released, the stream has ended)
  //
  // Return value: 0 if successful, an `errno`-compatible error code otherwise.
  //
  // If successful, the ArrowArray must be released independently from the stream.
  int (*get_next)(struct ArrowArrayStream*, struct ArrowArray* out);

  // Callback to get optional detailed error information.
  // This must only be called if the last stream operation failed
  // with a non-0 return code.
  //
  // Return value: pointer to a null-terminated character array describing
  // the last error, or NULL if no description is available.
  //
  // The returned pointer is only valid until the next operation on this stream
  // (including release).
  const char* (*get_last_error)(struct ArrowArrayStream*);

  // Release callback: release the stream's own resources.
  // Note that arrays returned by `get_next` must be individually released.
  void (*release)(struct ArrowArrayStream*);

  // Opaque producer-specific data
  void* private_data;
};

#endif  // ARROW_C_STREAM_INTERFACE
#endif  // ARROW_FLAG_DICTIONARY_ORDERED

typedef int GeoArrowFFIErrorCode;

struct GeoArrowFFIAllocator {
  GeoArrowFFIErrorCode (*reallocate)(struct GeoArrowFFIAllocator* self, uint8_t** ptr,
                                     int64_t old_size, int64_t new_size);
  void (*free)(struct GeoArrowFFIAllocator* self, uint8_t* ptr, int64_t old_size);
  void (*release)(struct GeoArrowFFIAllocator* self);
  void* private_data;
};

struct GeoArrowFFIArg {
  struct ArrowSchema* schema;
  struct ArrowArray* value;
};

struct GeoArrowFFIFunctionState {
  GeoArrowFFIErrorCode (*set_allocator)(GeoArrowFFIFunctionState* data);
  GeoArrowFFIErrorCode (*push_view)(GeoArrowFFIFunctionState* data,
                                    const ArrowArray* array, int64_t n_arrays);
  GeoArrowFFIErrorCode (*push_owned)(GeoArrowFFIFunctionState* data, ArrowArray* array,
                                     int64_t n_arrays);
  GeoArrowFFIErrorCode (*pull)(GeoArrowFFIFunctionState* data, struct ArrowArray* out);
  const char* (*get_last_error)(struct GeoArrowFFIFunctionState* data);
  void (*release)(struct GeoArrowFFIFunctionState* self);
  void* private_data;
};

struct GeoArrowFFIFunction {
  GeoArrowFFIErrorCode (*set_option)(struct GeoArrowFFIFunction* self, const char* key,
                                     const char* value);
  GeoArrowFFIErrorCode (*get_option)(struct GeoArrowFFIFunction* self, const char* key,
                                     const char** out_value);

  GeoArrowFFIErrorCode (*bind)(struct GeoArrowFFIFunction* self,
                               const struct GeoArrowFFIArgument* args, int64_t n_args,
                               const struct ArrowSchema** out_type,
                               struct GeoArrowFFIFunctionState* function_data_out);

  const char* (*get_last_error)(struct GeoArrowFFIFunction* self);

  void (*release)(struct GeoArrowFFIFunction* self);
  void* private_data;
};

#ifdef __cplusplus
}
#endif

#endif
