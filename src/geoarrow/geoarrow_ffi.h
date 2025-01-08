
#ifndef GEOARROW_FFI_INCLUDED
#define GEOARROW_FFI_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \defgroup geoarrow-ffi GeoArrow ABI-Stable structures
///
/// This header provides the ABI-stable structures used for within-process
/// interoperability among GeoArrow implementations.
///
/// @{

/// \brief An errno-compatbile error code
typedef int GeoArrowErrorCode;

/// \brief A function whose GeoArrowFFIFunctionState push/pull contract is not constrained
/// beyond those noted in the struct definition.
///
/// This may be used to prototype other types of functions or share other types of
/// functions whose contract is agreed upon outside this specification.
#define GEOARROW_FFI_FUNCTION_TYPE_UNKNOWN 0

/// \brief A function that applies a 1:1 input element to output element
///
/// A function requested from a GeoArrowFFICatalog with this value must implement a
/// GeoArrowFFIFunctionState where a single call to push is matched to a single call to
/// pull where the number of rows is the same.
#define GEOARROW_FFI_FUNCTION_TYPE_SCALAR 1

/// \brief A function whose output is a length 1 array regardless of the number of input
/// rows
///
/// A function requested from a GeoArrowFFICatalog with this value must implement a
/// GeoArrowFFIFunctionState whose pull callback always returns an ArrowArray with
/// a single row regardless of the number of preceding pull calls.
#define GEOARROW_FFI_FUNCTION_TYPE_ACCUMULATOR 2

/// \brief A function whose output does not depend on the size of the input
///
/// A function requested from a GeoArrowFFICatalog with this value must implement a
/// GeoArrowFFIFunctionState whose pull callback processes all previous calls to
/// push as a single logical array (e.g., a chunked array) and where the consumer
/// must pull until an released array is returned from the output. This may also
/// be used to implement a table function (e.g., where filenames are pushed as
/// arguments and table scans are performed via pull).
#define GEOARROW_FFI_FUNCTION_TYPE_ARRAY 4

/// @}

/// \defgroup arrow-ffi Arrow ABI-Stable structures
///
/// A group for the Arrow ABI to keep its documentation separate
///
/// @{

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

/// @}

/// \brief An allocator to use for large allocations
/// \ingroup geoarrow-ffi
///
/// Most engines track memory usage very closely and provide a custom allocator
/// to maximize performance. This structure allows the engine to specify how
/// array buffers or other large intermediary buffers should be allocated.
struct GeoArrowFFIAllocator {
  /// \brief Allocate a new buffer or reallocate an old one
  ///
  /// New buffers are requested by ptr pointing to NULL and old_size equal to zero.
  GeoArrowErrorCode (*reallocate)(struct GeoArrowFFIAllocator* self, uint8_t** ptr,
                                  int64_t old_size, int64_t new_size);

  /// \brief Free a buffer allocated by reallocate
  void (*free)(struct GeoArrowFFIAllocator* self, uint8_t* ptr, int64_t old_size);

  /// \brief Clone this allocator
  ///
  /// This will get
  GeoArrowErrorCode (*clone)(struct GeoArrowFFIAllocator* data,
                             struct GeoArrowFFIAllocator* copy);

  /// \brief Release any resources associated with this allocator and set the release
  /// callback to NULL.
  void (*release)(struct GeoArrowFFIAllocator* self);

  /// \brief Opaque, implementation-specific data
  void* private_data;
};

/// \brief State tracking a specific call to a function
/// \ingroup geoarrow-ffi
///
/// Specifically, this is the result of a call to "bind" (i.e., input types
/// and/or constant options are already known when this object is instantiated).
/// The callbacks provided by this object perform the actual computation.
/// Calls to these callbacks must be serialized/occur from the same thread; however,
/// a clone callback is provided for cases where the engine cannot meet this guarantee.
struct GeoArrowFFIFunctionState {
  /// \brief Set the allocator that should be used for buffer allocations
  ///
  /// Implementations are not required to implement this callback but not doing
  /// so may result in unexpected out-of-memory or degraded performance when used
  /// with an engine that expects its use.
  GeoArrowErrorCode (*set_allocator)(struct GeoArrowFFIFunctionState* data);

  /// \brief Populate an independent clone of this state that can be used concurrently
  ///
  /// Most engines do not organize their user-defined function interface in such a
  /// way that batch calls are guaranteed to occur on the same thread. In this case,
  /// an engine can clone a reference instance on each batch and use it to perform
  /// the computation. Because of this, implementations of this callback should be
  /// cheap.
  GeoArrowErrorCode (*clone)(struct GeoArrowFFIFunctionState* data,
                             struct GeoArrowFFIFunctionState* copy);

  /// \brief Push a batch of arguments into the compute function
  ///
  /// The arrays are interpreted according to the ArrowSchemas that were provided to the
  /// bind callback that produced this state. The implementation should not perform
  /// any computations in this step other than validating the structure of the input.
  ///
  /// Implementations for some functions may allow more than one batch to be pushed before
  /// a call to pull; however, the canonical pattern for simple scalar functions
  /// is a single pair of push/pull for each batch.
  ///
  /// The input arrays must have length of n_rows or 1. Constant values should be
  /// represented by an array of length 1.
  GeoArrowErrorCode (*push)(struct GeoArrowFFIFunctionState* data,
                            struct ArrowArray** arrays, int64_t n_arrays, int64_t n_rows);

  /// \brief Compute and retrieve results of a calling the function
  ///
  /// Perform computation and pull n_rows of result into out (or a released out
  /// to indicate there is no more output until another call to push).
  GeoArrowErrorCode (*pull)(struct GeoArrowFFIFunctionState* data, struct ArrowArray* out,
                            int64_t n_rows);

  /// \brief Retrieve a detailed error message from a previous erroring callback
  ///
  /// The result is valid until the next call to any callback, including release.
  const char* (*get_last_error)(struct GeoArrowFFIFunctionState* data);

  /// \brief Release any resources associated with this state and set the release
  /// callback to NULL.
  void (*release)(struct GeoArrowFFIFunctionState* self);

  /// \brief Opaque, implementation-specific data
  void* private_data;
};

/// \brief An instance of a compute function
/// \ingroup geoarrow-ffi
///
/// Typically there will be one instance of this object in some global registry
/// wrapped in the engine's function registry as a user-defined function of some
/// kind.
struct GeoArrowFFIFunction {
  /// \brief Bind a new instance of a GeoArrowFFIFunctionState and compute the output type
  ///
  /// This callback must be thread safe and reentrant (i.e., it can be called concurrently
  /// from more than one thread at a time to produce independent function states).
  /// This GeoArrowFFIFunction instance must outlive the lifetime of the populated
  /// GeoArrowFFIFunctionState.
  GeoArrowErrorCode (*bind)(struct GeoArrowFFIFunction* self, struct ArrowSchema** args,
                            int64_t n_args, const char* options,
                            struct ArrowSchema* out_schema,
                            struct GeoArrowFFIFunctionState* out_function_data);

  /// \brief Retrieve a detailed error message from a previous erroring callback
  ///
  /// The result is valid until the next call to any callback, including release.
  const char* (*get_last_error)(struct GeoArrowFFIFunction* self);

  /// \brief Release any resources associated with this state and set the release
  /// callback to NULL.
  void (*release)(struct GeoArrowFFIFunction* self);

  /// \brief Opaque, implementation-specific data
  void* private_data;
};

/// \brief A producer of GeoarrowFFIFunction instances
/// \ingroup geoarrow-ffi
struct GeoArrowFFICatalog {
  /// \brief Populate a GeoArrowFFIFunction of the requested type
  ///
  /// The function_type parameter corresponds to the GEOARROW_FFI_FUNCTION_TYPE_*
  /// definitions above; options are packed in the same way as the ArrowSchema::metadata
  /// field.
  GeoArrowErrorCode (*get_function)(int function_type, const char* name,
                                    const char* options,
                                    struct GeoArrowFFIFunction* out_function);

  /// \brief Retrieve a detailed error message from a previous erroring callback
  ///
  /// The result is valid until the next call to any callback, including release.
  const char* (*get_last_error)(struct GeoArrowFFIFunction* self);

  /// \brief Release any resources associated with this catalog and set the release
  /// callback to NULL.
  void (*release)(struct GeoArrowFFICatalog* self);

  /// \brief Opaque, implementation-specific data
  void* private_data;
};

/// @}

#ifdef __cplusplus
}
#endif

#endif
