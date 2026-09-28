
# cython: language_level = 3
# cython: linetrace=True
# cython: freethreading_compatible = True

"""Low-level geoarrow Python bindings."""

from libc.stdint cimport uint8_t, int32_t, int64_t, uintptr_t
from libc.stdlib cimport free, malloc
from cpython cimport Py_buffer, PyObject
from cpython.pycapsule cimport PyCapsule_GetPointer, PyCapsule_New
from libcpp cimport bool
from libcpp.string cimport string

cdef extern from "geoarrow_type.h":
    struct ArrowSchema:
        const char* format
        const char* name
        const char* metadata
        int64_t flags
        int64_t n_children
        ArrowSchema** children
        ArrowSchema* dictionary
        void (*release)(ArrowSchema*)
        void* private_data

    struct ArrowArray:
        int64_t length
        int64_t null_count
        int64_t offset
        int64_t n_buffers
        int64_t n_children
        const void** buffers
        ArrowArray** children
        ArrowArray* dictionary
        void (*release)(ArrowArray*)
        void* private_data

    struct ArrowArrayStream:
        int (*get_schema)(ArrowArrayStream*, ArrowSchema* out)
        int (*get_next)(ArrowArrayStream*, ArrowArray* out)
        const char* (*get_last_error)(ArrowArrayStream*)
        void (*release)(ArrowArrayStream*)
        void* private_data

    ctypedef int GeoArrowErrorCode
    cdef int GEOARROW_OK

    cpdef enum GeoArrowGeometryType:
        GEOARROW_GEOMETRY_TYPE_GEOMETRY = 0
        GEOARROW_GEOMETRY_TYPE_POINT = 1
        GEOARROW_GEOMETRY_TYPE_LINESTRING = 2
        GEOARROW_GEOMETRY_TYPE_POLYGON = 3
        GEOARROW_GEOMETRY_TYPE_MULTIPOINT = 4
        GEOARROW_GEOMETRY_TYPE_MULTILINESTRING = 5
        GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON = 6
        GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION = 7
        GEOARROW_GEOMETRY_TYPE_BOX = 990

    cpdef enum GeoArrowDimensions:
        GEOARROW_DIMENSIONS_UNKNOWN = 0
        GEOARROW_DIMENSIONS_XY = 1
        GEOARROW_DIMENSIONS_XYZ = 2
        GEOARROW_DIMENSIONS_XYM = 3
        GEOARROW_DIMENSIONS_XYZM = 4

    cpdef enum GeoArrowCoordType:
        GEOARROW_COORD_TYPE_UNKNOWN = 0
        GEOARROW_COORD_TYPE_SEPARATE = 1
        GEOARROW_COORD_TYPE_INTERLEAVED = 2

    cpdef enum GeoArrowType:
        GEOARROW_TYPE_UNINITIALIZED = 0
        GEOARROW_TYPE_WKB = 100001
        GEOARROW_TYPE_LARGE_WKB = 100002
        GEOARROW_TYPE_WKT = 100003
        GEOARROW_TYPE_LARGE_WKT = 100004
        GEOARROW_TYPE_WKB_VIEW = 100005
        GEOARROW_TYPE_WKT_VIEW = 100006
        GEOARROW_TYPE_BOX_Z = 1990
        GEOARROW_TYPE_BOX_M = 2990
        GEOARROW_TYPE_BOX_ZM = 3990
        GEOARROW_TYPE_POINT = 1
        GEOARROW_TYPE_LINESTRING = 2
        GEOARROW_TYPE_POLYGON = 3
        GEOARROW_TYPE_MULTIPOINT = 4
        GEOARROW_TYPE_MULTILINESTRING = 5
        GEOARROW_TYPE_MULTIPOLYGON = 6
        GEOARROW_TYPE_POINT_Z = 1001
        GEOARROW_TYPE_LINESTRING_Z = 1002
        GEOARROW_TYPE_POLYGON_Z = 1003
        GEOARROW_TYPE_MULTIPOINT_Z = 1004
        GEOARROW_TYPE_MULTILINESTRING_Z = 1005
        GEOARROW_TYPE_MULTIPOLYGON_Z = 1006
        GEOARROW_TYPE_POINT_M = 2001
        GEOARROW_TYPE_LINESTRING_M = 2002
        GEOARROW_TYPE_POLYGON_M = 2003
        GEOARROW_TYPE_MULTIPOINT_M = 2004
        GEOARROW_TYPE_MULTILINESTRING_M = 2005
        GEOARROW_TYPE_MULTIPOLYGON_M = 2006
        GEOARROW_TYPE_POINT_ZM = 3001
        GEOARROW_TYPE_LINESTRING_ZM = 3002
        GEOARROW_TYPE_POLYGON_ZM = 3003
        GEOARROW_TYPE_MULTIPOINT_ZM = 3004
        GEOARROW_TYPE_MULTILINESTRING_ZM = 3005
        GEOARROW_TYPE_MULTIPOLYGON_ZM = 3006
        GEOARROW_TYPE_INTERLEAVED_POINT = 10001
        GEOARROW_TYPE_INTERLEAVED_LINESTRING = 10002
        GEOARROW_TYPE_INTERLEAVED_POLYGON = 10003
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT = 10004
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING = 10005
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON = 10006
        GEOARROW_TYPE_INTERLEAVED_POINT_Z = 11001
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z = 11002
        GEOARROW_TYPE_INTERLEAVED_POLYGON_Z = 11003
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_Z = 11004
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_Z = 11005
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_Z = 11006
        GEOARROW_TYPE_INTERLEAVED_POINT_M = 12001
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_M = 12002
        GEOARROW_TYPE_INTERLEAVED_POLYGON_M = 12003
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_M = 12004
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_M = 12005
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_M = 12006
        GEOARROW_TYPE_INTERLEAVED_POINT_ZM = 13001
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_ZM = 13002
        GEOARROW_TYPE_INTERLEAVED_POLYGON_ZM = 13003
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_ZM = 13004
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_ZM = 13005
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_ZM = 13006

    cpdef enum GeoArrowEdgeType:
        GEOARROW_EDGE_TYPE_PLANAR
        GEOARROW_EDGE_TYPE_SPHERICAL
        GEOARROW_EDGE_TYPE_VINCENTY
        GEOARROW_EDGE_TYPE_THOMAS
        GEOARROW_EDGE_TYPE_ANDOYER
        GEOARROW_EDGE_TYPE_KARNEY

    cpdef enum GeoArrowCrsType:
        GEOARROW_CRS_TYPE_NONE
        GEOARROW_CRS_TYPE_UNKNOWN
        GEOARROW_CRS_TYPE_PROJJSON
        GEOARROW_CRS_TYPE_WKT2_2019
        GEOARROW_CRS_TYPE_AUTHORITY_CODE
        GEOARROW_CRS_TYPE_SRID

    struct GeoArrowError:
        char message[1024]

    struct GeoArrowStringView:
        const char* data
        int64_t size_bytes

    struct GeoArrowBufferView:
        const uint8_t* data
        int64_t size_bytes

    struct GeoArrowSchemaView:
        ArrowSchema* schema
        GeoArrowStringView extension_name
        GeoArrowStringView extension_metadata
        GeoArrowType type
        GeoArrowGeometryType geometry_type
        GeoArrowDimensions dimensions
        GeoArrowCoordType coord_type

    struct GeoArrowMetadataView:
        GeoArrowStringView metadata
        GeoArrowEdgeType edge_type
        GeoArrowCrsType crs_type
        GeoArrowStringView crs

    struct GeoArrowKernel:
        int (*start)(GeoArrowKernel* kernel, ArrowSchema* schema,
                     const char* options, ArrowSchema* out, GeoArrowError* error)
        int (*push_batch)(GeoArrowKernel* kernel, ArrowArray* array,
                          ArrowArray* out, GeoArrowError* error) nogil
        int (*finish)(GeoArrowKernel* kernel, ArrowArray* out,
                      GeoArrowError* error) nogil
        void (*release)(GeoArrowKernel* kernel)
        void* private_data

    struct GeoArrowCoordView:
        const double* values[4]
        int64_t n_coords
        int32_t n_values
        int32_t coords_stride

    struct GeoArrowArrayView:
        GeoArrowSchemaView schema_view
        int64_t offset[4]
        int64_t length[4]
        const uint8_t* validity_bitmap
        int32_t n_offsets
        const int32_t* offsets[3]
        int32_t first_offset[3]
        int32_t last_offset[3]
        GeoArrowCoordView coords



    struct GeoArrowBuilder:
        pass


cdef extern from "geoarrow.h":
    GeoArrowErrorCode GeoArrowKernelInit(GeoArrowKernel* kernel, const char* name, const char* options)

    GeoArrowErrorCode GeoArrowArrayViewInitFromSchema(GeoArrowArrayView* array_view,
                                                      ArrowSchema* schema,
                                                      GeoArrowError* error)

    GeoArrowErrorCode GeoArrowArrayViewSetArray(GeoArrowArrayView* array_view,
                                                ArrowArray* array,
                                                GeoArrowError* error)

    GeoArrowErrorCode GeoArrowBuilderInitFromSchema(GeoArrowBuilder* builder,
                                                    ArrowSchema* schema,
                                                    GeoArrowError* error)

    GeoArrowErrorCode GeoArrowBuilderAppendBuffer(
        GeoArrowBuilder* builder, int64_t i, GeoArrowBufferView value)

    void GeoArrowBuilderReset(GeoArrowBuilder* builder)

    GeoArrowErrorCode GeoArrowBuilderFinish(GeoArrowBuilder* builder,
                                            ArrowArray* array,
                                            GeoArrowError* error)


cdef extern from "nanoarrow/nanoarrow.h":
    void ArrowArrayMove(ArrowArray* src, ArrowArray* dst)
    void ArrowArrayStreamMove(ArrowArrayStream* src, ArrowArrayStream* dst)
    void ArrowSchemaMove(ArrowSchema* src, ArrowSchema* dst)
    GeoArrowErrorCode ArrowSchemaDeepCopy(ArrowSchema* schema,
                                          ArrowSchema* schema_out)
    GeoArrowErrorCode ArrowBasicArrayStreamInit(
        ArrowArrayStream* array_stream, ArrowSchema* schema, int64_t n_arrays)
    void ArrowBasicArrayStreamSetArray(
        ArrowArrayStream* array_stream, int64_t i, ArrowArray* array)

cdef extern from "geoarrow_python.h":

    GeoArrowErrorCode GeoArrowBuilderSetPyBuffer(GeoArrowBuilder* builder, int64_t i, PyObject* obj,
                                                 const void* ptr, int64_t size)


cdef extern from "geoarrow.hpp" namespace "geoarrow":
    cdef cppclass GeometryDataType:
        GeometryDataType() except +ValueError
        GeometryDataType(const GeometryDataType& x) except +ValueError
        void MoveFrom(GeometryDataType* other)

        string extension_name() except +ValueError
        string extension_metadata() except +ValueError
        GeoArrowType id()
        GeoArrowGeometryType geometry_type()
        GeoArrowDimensions dimensions()
        GeoArrowCoordType coord_type()
        int num_dimensions()
        GeoArrowEdgeType edge_type()
        GeoArrowCrsType crs_type()
        string crs()
        string ToString()

        GeometryDataType WithGeometryType(GeoArrowGeometryType geometry_type) except +ValueError
        GeometryDataType WithCoordType(GeoArrowCoordType coord_type) except +ValueError
        GeometryDataType WithDimensions(GeoArrowDimensions dimensions) except +ValueError
        GeometryDataType WithEdgeType(GeoArrowEdgeType edge_type) except +ValueError
        GeometryDataType WithCrs(const string& crs, GeoArrowCrsType crs_type) except +ValueError

        void InitSchema(ArrowSchema* schema) except +ValueError
        void InitStorageSchema(ArrowSchema* schema) except +ValueError

        @staticmethod
        GeometryDataType Make0 "Make"(GeoArrowGeometryType geometry_type,
                                GeoArrowDimensions dimensions,
                                GeoArrowCoordType coord_type,
                                const string& metadata) except +ValueError

        @staticmethod
        GeometryDataType MakeType "Make"(GeoArrowType type,
                                          const string& metadata) except +ValueError

        @staticmethod
        GeometryDataType Make1 "Make"(ArrowSchema* schema) except +ValueError

        @staticmethod
        GeometryDataType Make2 "Make"(ArrowSchema* schema, const string& extension_name,
                                const string& metadata) except +ValueError


# The default changed in Cython 3.1.0 such that the member of an
# enum are no longer automatically copied to the parent module.
globals().update(getattr(GeoArrowGeometryType, '__members__'))
globals().update(getattr(GeoArrowDimensions, '__members__'))
globals().update(getattr(GeoArrowCoordType, '__members__'))
globals().update(getattr(GeoArrowType, '__members__'))
globals().update(getattr(GeoArrowEdgeType, '__members__'))
globals().update(getattr(GeoArrowCrsType, '__members__'))


class GeoArrowCException(RuntimeError):

    def __init__(self, what, code, message=""):
        self.what = what
        self.code = code
        self.message = message

        if self.message == "":
            super().__init__(f"{self.what} failed ({self.code})")
        else:
            super().__init__(f"{self.what} failed ({self.code}): {self.message}")


cdef void pycapsule_schema_deleter(object schema_capsule) noexcept:
    cdef ArrowSchema* schema = <ArrowSchema*>PyCapsule_GetPointer(
        schema_capsule, "arrow_schema"
    )
    if schema == NULL:
        return

    if schema.release != NULL:
        schema.release(schema)

    free(schema)


cdef void pycapsule_array_deleter(object array_capsule) noexcept:
    cdef ArrowArray* array = <ArrowArray*>PyCapsule_GetPointer(
        array_capsule, "arrow_array"
    )
    if array == NULL:
        return

    if array.release != NULL:
        array.release(array)

    free(array)


cdef void pycapsule_array_stream_deleter(object stream_capsule) noexcept:
    cdef ArrowArrayStream* stream = <ArrowArrayStream*>PyCapsule_GetPointer(
        stream_capsule, "arrow_array_stream"
    )
    if stream == NULL:
        return

    if stream.release != NULL:
        stream.release(stream)

    free(stream)


cdef class Error:
    cdef GeoArrowError c_error

    def __cinit__(self):
        self.c_error.message[0] = 0

    def raise_message(self, what, code):
        raise GeoArrowCException(what, code, self.c_error.message.decode("UTF-8"))

    @staticmethod
    def raise_error(what, code):
        raise GeoArrowCException(what, code, "")


cdef class SchemaHolder:
    cdef ArrowSchema c_schema

    def __cinit__(self):
        self.c_schema.release = NULL

    def __dealloc__(self):
        if self.c_schema.release != NULL:
          self.c_schema.release(&self.c_schema)

    def _addr(self):
        return <uintptr_t>&self.c_schema

    def __arrow_c_schema__(self):
        """Export an independent Arrow schema capsule."""
        if self.c_schema.release == NULL:
            raise ValueError("Schema is already released")

        cdef ArrowSchema* schema = <ArrowSchema*>malloc(sizeof(ArrowSchema))
        if schema == NULL:
            raise MemoryError()

        schema.release = NULL
        capsule = PyCapsule_New(
            schema, "arrow_schema", &pycapsule_schema_deleter
        )
        cdef int result = ArrowSchemaDeepCopy(&self.c_schema, schema)
        if result != GEOARROW_OK:
            Error.raise_error("ArrowSchemaDeepCopy()", result)

        return capsule

    @staticmethod
    def from_arrow_c_schema(obj):
        """Import an Arrow schema capsule or ``__arrow_c_schema__`` provider."""
        if hasattr(obj, "__arrow_c_schema__"):
            obj = obj.__arrow_c_schema__()

        cdef ArrowSchema* schema = <ArrowSchema*>PyCapsule_GetPointer(
            obj, "arrow_schema"
        )
        if schema.release == NULL:
            raise ValueError("Arrow schema is released")

        out = SchemaHolder()
        ArrowSchemaMove(schema, &out.c_schema)
        return out

    def is_valid(self):
        return self.c_schema.release != NULL

    def release(self):
        if self.c_schema.release == NULL:
            raise ValueError('Schema is already released')
        self.c_schema.release(&self.c_schema)


cdef class ArrayHolder:
    cdef ArrowArray c_array
    cdef object _schema

    def __cinit__(self):
        self.c_array.release = NULL
        self._schema = None

    def __dealloc__(self):
        if self.c_array.release != NULL:
          self.c_array.release(&self.c_array)

    def _addr(self):
        return <uintptr_t>&self.c_array

    def get_schema(self):
        """Return an independent copy of this array's schema."""
        if self._schema is None:
            raise ValueError("Array holder does not have a schema")
        return SchemaHolder.from_arrow_c_schema(self._schema)

    def __arrow_c_array__(self, requested_schema=None):
        """Export this array using the Arrow PyCapsule protocol."""
        if requested_schema is not None:
            raise ValueError("Requested schema is not supported")
        if self.c_array.release == NULL:
            raise ValueError("Array is already released")
        if self._schema is None:
            raise ValueError("Array holder does not have a schema")

        cdef ArrowArray* array = <ArrowArray*>malloc(sizeof(ArrowArray))
        if array == NULL:
            raise MemoryError()

        array.release = NULL
        capsule = PyCapsule_New(
            array, "arrow_array", &pycapsule_array_deleter
        )
        ArrowArrayMove(&self.c_array, array)
        return self._schema.__arrow_c_schema__(), capsule

    @staticmethod
    def from_arrow_c_array(obj):
        """Import an ``__arrow_c_array__`` provider."""
        if not hasattr(obj, "__arrow_c_array__"):
            raise TypeError("Expected an __arrow_c_array__ provider")

        capsules = obj.__arrow_c_array__()
        if not isinstance(capsules, tuple) or len(capsules) != 2:
            raise TypeError(
                "__arrow_c_array__ must return a schema and array capsule"
            )

        cdef ArrowArray* array = <ArrowArray*>PyCapsule_GetPointer(
            capsules[1], "arrow_array"
        )
        if array == NULL:
            raise ValueError("Invalid Arrow array capsule")
        if array.release == NULL:
            raise ValueError("Arrow array is released")

        out = ArrayHolder()
        out._schema = SchemaHolder.from_arrow_c_schema(capsules[0])
        ArrowArrayMove(array, &out.c_array)
        return out

    def is_valid(self):
        return self.c_array.release != NULL

    def release(self):
        if self.c_array.release == NULL:
            raise ValueError('Array is already released')
        self.c_array.release(&self.c_array)


cdef class ArrayStreamHolder:
    cdef ArrowArrayStream c_stream
    cdef object _schema

    def __cinit__(self):
        self.c_stream.release = NULL
        self._schema = None

    def __dealloc__(self):
        if self.c_stream.release != NULL:
            self.c_stream.release(&self.c_stream)

    def __arrow_c_stream__(self, requested_schema=None):
        """Export this stream using the Arrow PyCapsule protocol."""
        if requested_schema is not None:
            raise ValueError("Requested schema is not supported")
        if self.c_stream.release == NULL:
            raise ValueError("Array stream is already released")

        cdef ArrowArrayStream* stream = <ArrowArrayStream*>malloc(
            sizeof(ArrowArrayStream)
        )
        if stream == NULL:
            raise MemoryError()

        stream.release = NULL
        capsule = PyCapsule_New(
            stream, "arrow_array_stream", &pycapsule_array_stream_deleter
        )
        ArrowArrayStreamMove(&self.c_stream, stream)
        return capsule

    @staticmethod
    def from_arrow_c_stream(obj):
        """Import an ``__arrow_c_stream__`` provider."""
        if not hasattr(obj, "__arrow_c_stream__"):
            raise TypeError("Expected an __arrow_c_stream__ provider")

        capsule = obj.__arrow_c_stream__()
        cdef ArrowArrayStream* stream = <ArrowArrayStream*>PyCapsule_GetPointer(
            capsule, "arrow_array_stream"
        )
        if stream == NULL:
            raise ValueError("Invalid Arrow array stream capsule")
        if stream.release == NULL:
            raise ValueError("Arrow array stream is released")

        out = ArrayStreamHolder()
        ArrowArrayStreamMove(stream, &out.c_stream)
        out._schema = out.get_schema()
        return out

    @staticmethod
    def from_arrays(SchemaHolder schema, arrays):
        """Create a stream by consuming a sequence of ``ArrayHolder`` objects."""
        schema_copy = SchemaHolder()
        cdef int result = ArrowSchemaDeepCopy(
            &schema.c_schema, &schema_copy.c_schema
        )
        if result != GEOARROW_OK:
            Error.raise_error("ArrowSchemaDeepCopy()", result)

        out = ArrayStreamHolder()
        result = ArrowBasicArrayStreamInit(
            &out.c_stream, &schema_copy.c_schema, len(arrays)
        )
        if result != GEOARROW_OK:
            Error.raise_error("ArrowBasicArrayStreamInit()", result)

        for i, array in enumerate(arrays):
            if not isinstance(array, ArrayHolder):
                raise TypeError("Expected an ArrayHolder")
            ArrowBasicArrayStreamSetArray(
                &out.c_stream, i, &(<ArrayHolder>array).c_array
            )

        out._schema = out.get_schema()
        return out

    def get_schema(self):
        """Return this stream's schema."""
        if self.c_stream.release == NULL:
            raise ValueError("Array stream is already released")

        out = SchemaHolder()
        cdef int result = self.c_stream.get_schema(
            &self.c_stream, &out.c_schema
        )
        if result != GEOARROW_OK:
            self._raise_last_error("ArrowArrayStream.get_schema()", result)
        return out

    def get_next(self):
        """Return the next ``ArrayHolder``, or ``None`` at end of stream."""
        if self.c_stream.release == NULL:
            raise ValueError("Array stream is already released")

        out = ArrayHolder()
        cdef int result = self.c_stream.get_next(
            &self.c_stream, &out.c_array
        )
        if result != GEOARROW_OK:
            self._raise_last_error("ArrowArrayStream.get_next()", result)
        if out.c_array.release == NULL:
            return None

        out._schema = self._schema
        return out

    def _raise_last_error(self, what, code):
        cdef const char* message = NULL
        if self.c_stream.get_last_error != NULL:
            message = self.c_stream.get_last_error(&self.c_stream)

        if message == NULL:
            raise GeoArrowCException(what, code)
        raise GeoArrowCException(what, code, message.decode("UTF-8"))

    def is_valid(self):
        return self.c_stream.release != NULL

    def release(self):
        if self.c_stream.release == NULL:
            raise ValueError("Array stream is already released")
        self.c_stream.release(&self.c_stream)


cdef class CGeometryDataType:
    cdef GeometryDataType c_vector_type

    def __cinit__(self):
        pass

    def __repr__(self):
        if self.c_vector_type.id() == GEOARROW_TYPE_UNINITIALIZED:
            return "<Uninitialized CGeometryDataType>"

        return self.c_vector_type.ToString().decode()

    @staticmethod
    cdef _move_from_ctype(GeometryDataType* c_vector_type):
        if c_vector_type.id() == GEOARROW_TYPE_UNINITIALIZED:
            raise ValueError("Uninitialized CGeometryDataType")
        out = CGeometryDataType()
        out.c_vector_type.MoveFrom(c_vector_type)
        return out

    def _assert_valid(self):
        if self.c_vector_type.id() == GEOARROW_TYPE_UNINITIALIZED:
            raise ValueError("Uninitialized CGeometryDataType")

    @property
    def id(self):
        self._assert_valid()
        return self.c_vector_type.id()

    @property
    def geometry_type(self):
        self._assert_valid()
        return self.c_vector_type.geometry_type()

    @property
    def dimensions(self):
        self._assert_valid()
        return self.c_vector_type.dimensions()

    @property
    def coord_type(self):
        self._assert_valid()
        return self.c_vector_type.coord_type()

    @property
    def extension_name(self):
        self._assert_valid()
        return self.c_vector_type.extension_name().decode("UTF-8")

    @property
    def extension_metadata(self):
        self._assert_valid()
        return self.c_vector_type.extension_metadata()

    @property
    def edge_type(self):
        self._assert_valid()
        return self.c_vector_type.edge_type()

    @property
    def crs_type(self):
        self._assert_valid()
        return self.c_vector_type.crs_type()

    @property
    def crs(self):
        self._assert_valid()
        return self.c_vector_type.crs()

    def with_geometry_type(self, GeoArrowGeometryType geometry_type):
        self._assert_valid()
        cdef GeometryDataType ctype = self.c_vector_type.WithGeometryType(geometry_type)
        return CGeometryDataType._move_from_ctype(&ctype)

    def with_dimensions(self, GeoArrowDimensions dimensions):
        self._assert_valid()
        cdef GeometryDataType ctype = self.c_vector_type.WithDimensions(dimensions)
        return CGeometryDataType._move_from_ctype(&ctype)

    def with_coord_type(self, GeoArrowCoordType coord_type):
        self._assert_valid()
        cdef GeometryDataType ctype = self.c_vector_type.WithCoordType(coord_type)
        return CGeometryDataType._move_from_ctype(&ctype)

    def with_edge_type(self, GeoArrowEdgeType edge_type):
        self._assert_valid()
        cdef GeometryDataType ctype = self.c_vector_type.WithEdgeType(edge_type)
        return CGeometryDataType._move_from_ctype(&ctype)

    def with_crs(self, string crs, GeoArrowCrsType crs_type):
        self._assert_valid()
        cdef GeometryDataType ctype = self.c_vector_type.WithCrs(crs, crs_type)
        return CGeometryDataType._move_from_ctype(&ctype)

    def __eq__(self, other):
        if not isinstance(other, CGeometryDataType):
            return False
        if self.id != other.id or self.edge_type != other.edge_type:
            return False
        if self.crs_type == other.crs_type and self.crs != other.crs:
            return False

        return self.crs_type == other.crs_type

    def to_schema(self):
        self._assert_valid()
        out = SchemaHolder()
        self.c_vector_type.InitSchema(&out.c_schema)
        return out

    def to_storage_schema(self):
        self._assert_valid()
        out = SchemaHolder()
        self.c_vector_type.InitStorageSchema(&out.c_schema)
        return out

    @staticmethod
    def Make(GeoArrowGeometryType geometry_type,
             GeoArrowDimensions dimensions,
             GeoArrowCoordType coord_type,
             metadata=b''):
        cdef GeometryDataType ctype = GeometryDataType.Make0(geometry_type, dimensions, coord_type, metadata)
        return CGeometryDataType._move_from_ctype(&ctype)

    @staticmethod
    def MakeType(GeoArrowType type, metadata=b''):
        cdef GeometryDataType ctype = GeometryDataType.MakeType(type, metadata)
        return CGeometryDataType._move_from_ctype(&ctype)

    @staticmethod
    def FromExtension(SchemaHolder schema):
        cdef GeometryDataType ctype = GeometryDataType.Make1(&schema.c_schema)
        return CGeometryDataType._move_from_ctype(&ctype)

    @staticmethod
    def FromStorage(SchemaHolder schema, string extension_name, string extension_metadata):
        cdef GeometryDataType ctype = GeometryDataType.Make2(&schema.c_schema, extension_name, extension_metadata)
        return CGeometryDataType._move_from_ctype(&ctype)

cdef class CKernel:
    cdef GeoArrowKernel c_kernel
    cdef object cname_str
    cdef object output_schema

    def __cinit__(self, const char* name):
        cdef const char* cname = <const char*>name
        self.cname_str = cname.decode("UTF-8")
        cdef int result = GeoArrowKernelInit(&self.c_kernel, cname, NULL)
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowKernelInit('{self.cname_str}'>)", result)

    def __dealloc__(self):
        if self.c_kernel.release != NULL:
            self.c_kernel.release(&self.c_kernel)

    def start(self, SchemaHolder schema, const char* options):
        cdef Error error = Error()
        out = SchemaHolder()
        cdef int result = self.c_kernel.start(&self.c_kernel, &schema.c_schema,
                                              options, &out.c_schema, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::start()", result)

        self.output_schema = out
        return out

    def push_batch(self, ArrayHolder array):
        cdef Error error = Error()
        out = ArrayHolder()
        cdef int result
        with nogil:
            result = self.c_kernel.push_batch(&self.c_kernel, &array.c_array,
                                              &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::push_batch()", result)

        out._schema = self.output_schema
        return out

    def finish(self):
        cdef Error error = Error()
        out = ArrayHolder()
        cdef int result
        with nogil:
            result = self.c_kernel.finish(&self.c_kernel, &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::finish()", result)

    def push_batch_agg(self, ArrayHolder array):
        cdef Error error = Error()
        cdef int result = self.c_kernel.push_batch(&self.c_kernel, &array.c_array,
                                                   NULL, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::push_batch()", result)

    def finish_agg(self):
        cdef Error error = Error()
        out = ArrayHolder()
        cdef int result = self.c_kernel.finish(&self.c_kernel, &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::finish()", result)

        out._schema = self.output_schema
        return out


cdef class CArrayView:
    cdef GeoArrowArrayView c_array_view
    cdef object _base

    def __cinit__(self, ArrayHolder array, SchemaHolder schema):
        self._base = array

        cdef Error error = Error()
        cdef int result = GeoArrowArrayViewInitFromSchema(&self.c_array_view, &schema.c_schema, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message("GeoArrowArrayViewInitFromSchema()", result)

        result = GeoArrowArrayViewSetArray(&self.c_array_view, &array.c_array, &error.c_error)
        if result != GEOARROW_OK:
            raise ValueError(error.message.decode('UTF-8'))

    def buffers(self):
        buffers = []
        cdef int64_t length

        # Validity not quite implemented
        buf = None
        buffers.append(buf)

        if self.c_array_view.n_offsets > 0:
            buf = CArrayViewBuffer(
                self,
                <uintptr_t>self.c_array_view.offsets[0],
                4,
                self.c_array_view.offset[0] + self.c_array_view.length[0] + 1,
                'i'
            )
            buffers.append(buf)

        if self.c_array_view.n_offsets > 1:
            for i in range(self.c_array_view.n_offsets - 1):
                length = self.c_array_view.last_offset[i]
                buf = CArrayViewBuffer(
                    self,
                    <uintptr_t>self.c_array_view.offsets[i + 1],
                    4,
                    length + 1,
                    'i'
                )
            buffers.append(buf)

        cdef GeoArrowCoordView* coords = &self.c_array_view.coords
        if coords.coords_stride == 1:
            for i in range(coords.n_values):
                buf = CArrayViewBuffer(
                    self,
                    <uintptr_t>coords.values[i],
                    8,
                    coords.n_coords,
                    'd'
                )
                buffers.append(buf)
        elif coords.coords_stride == coords.n_values:
            buf = CArrayViewBuffer(
                self,
                <uintptr_t>coords.values[0],
                8,
                coords.n_coords * coords.n_values,
                'd'
            )
            buffers.append(buf)
        else:
            raise NotImplementedError('Unknown coord type')

        return buffers


cdef class CArrayViewBuffer:
    cdef object _base
    cdef void* _ptr
    cdef Py_ssize_t _item_size
    cdef Py_ssize_t _shape
    cdef str _format

    def __cinit__(self, base, uintptr_t ptr, item_size_bytes, length_elements, format):
        self._base = base
        self._ptr = <void*>ptr
        self._item_size = item_size_bytes
        self._shape = length_elements
        self._format = format

    def __getbuffer__(self, Py_buffer *buffer, int flags):
        buffer.buf = self._ptr

        if self._format == 'i':
            buffer.format = 'i'
        elif self._format == 'd':
            buffer.format = 'd'
        else:
            buffer.format = NULL

        buffer.internal = NULL
        buffer.itemsize = self._item_size
        buffer.len = self._shape * self._item_size
        buffer.ndim = 1
        buffer.obj = self
        buffer.readonly = 1
        buffer.shape = &self._shape
        buffer.strides = &self._item_size
        buffer.suboffsets = NULL

    def __releasebuffer__(self, Py_buffer *buffer):
        pass


cdef class CBuilder:
    cdef GeoArrowBuilder c_builder
    cdef SchemaHolder _schema

    def __cinit__(self, SchemaHolder schema):
        self._schema = schema
        cdef Error error = Error()
        cdef int result = GeoArrowBuilderInitFromSchema(&self.c_builder, &schema.c_schema, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message("GeoArrowBuilderInitFromSchema()", result)

    def __dealloc__(self):
        GeoArrowBuilderReset(&self.c_builder)

    def set_buffer_uint8(self, int64_t i, object obj):
        cdef const unsigned char[:] view = memoryview(obj)
        cdef int result = GeoArrowBuilderSetPyBuffer(&self.c_builder, i, <PyObject*>obj, &(view[0]), view.shape[0])
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowBuilderSetPyBuffer()", result)

    def set_buffer_int32(self, int64_t i, object obj):
        cdef const int32_t[:] view = memoryview(obj)
        cdef int result = GeoArrowBuilderSetPyBuffer(&self.c_builder, i, <PyObject*>obj, &(view[0]), view.shape[0] * 4)
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowBuilderSetPyBuffer()", result)

    def set_buffer_double(self, int64_t i, object obj):
        cdef const double[:] view = memoryview(obj)
        cdef int result = GeoArrowBuilderSetPyBuffer(&self.c_builder, i, <PyObject*>obj, &(view[0]), view.shape[0] * 8)
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowBuilderSetPyBuffer()", result)

    @property
    def schema(self):
        return self._schema

    def finish(self):
        out = ArrayHolder()
        cdef Error error = Error()
        cdef int result = GeoArrowBuilderFinish(&self.c_builder, &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message("GeoArrowBuilderFinish()", result)
        return out
