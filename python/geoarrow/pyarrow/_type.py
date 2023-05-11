import pyarrow as pa

from .. import lib


class VectorType(pa.ExtensionType):
    _extension_name = None

    # These are injected into the class when imported by the type and scalar
    # modules to avoid a circular import. As a result, you can't
    # use this module directly (import geoarrow.pyarrow first).
    _array_cls_from_name = None
    _scalar_cls_from_name = None
    _make_validate_kernel = None

    def __init__(self, c_vector_type):
        if not isinstance(c_vector_type, lib.CVectorType):
            raise TypeError(
                "geoarrow.pyarrow.VectorType must be created from a CVectorType"
            )
        self._type = c_vector_type
        if self._type.extension_name != type(self)._extension_name:
            raise ValueError(
                f'Expected CVectorType with extension name "{type(self)._extension_name}" but got "{self._type.extension_name}"'
            )

        storage_schema = self._type.to_storage_schema()
        storage_type = pa.DataType._import_from_c(storage_schema._addr())
        pa.ExtensionType.__init__(self, storage_type, self._type.extension_name)

    def __repr__(self):
        return f"{type(self).__name__}({repr(self._type)})"

    def __arrow_ext_serialize__(self):
        return self._type.extension_metadata

    @staticmethod
    def _import_from_c(addr):
        field = pa.Field._import_from_c(addr)
        if not field.metadata or "ARROW:extension:name" not in field.metadata:
            return field.type

        schema = lib.SchemaHolder()
        field._export_to_c(schema._addr())

        c_vector_type = lib.CVectorType.FromExtension(schema)
        cls = type_cls_from_name(c_vector_type.extension_name.decode("UTF-8"))
        cls(c_vector_type)

    @classmethod
    def __arrow_ext_deserialize__(cls, storage_type, serialized):
        schema = lib.SchemaHolder()
        storage_type._export_to_c(schema._addr())

        c_vector_type = lib.CVectorType.FromStorage(
            schema, cls._extension_name.encode("UTF-8"), serialized
        )

        return cls(c_vector_type)

    def wrap_array(self, obj, validate=False):
        out = super().wrap_array(obj)
        if validate:
            validator = VectorType._make_validate_kernel(self)
            validator.push(out)
            validator.finish()

        return out

    def __arrow_ext_class__(self):
        return VectorType._array_cls_from_name(self.extension_name)

    def __arrow_ext_scalar_class__(self):
        return VectorType._scalar_cls_from_name(self.extension_name)

    def from_geobuffers(self, *args, **kwargs):
        raise NotImplementedError()

    def _from_geobuffers_internal(self, args):
        builder = lib.CBuilder(self._type.to_schema())
        for i, buf_type, buf in args:
            if buf is None:
                continue
            if buf_type == "uint8":
                builder.set_buffer_uint8(i, buf)
            elif buf_type == "int32":
                builder.set_buffer_int32(i, buf)
            elif buf_type == "double":
                builder.set_buffer_double(i, buf)
            else:
                raise ValueError(f"Unknown type: {buf_type}")

        carray = builder.finish()
        return pa.Array._import_from_c(carray._addr(), self)

    @property
    def id(self):
        return self._type.id

    @property
    def geometry_type(self):
        return self._type.geometry_type

    @property
    def dimensions(self):
        return self._type.dimensions

    @property
    def coord_type(self):
        return self._type.coord_type

    @property
    def edge_type(self):
        return self._type.edge_type

    @property
    def crs_type(self):
        return self._type.crs_type

    @property
    def crs(self):
        return self._type.crs.decode("UTF-8")

    def with_geometry_type(self, geometry_type):
        ctype = self._type.with_geometry_type(geometry_type)
        return _ctype_to_extension_type(ctype)

    def with_dimensions(self, dimensions):
        ctype = self._type.with_dimensions(dimensions)
        return _ctype_to_extension_type(ctype)

    def with_coord_type(self, coord_type):
        ctype = self._type.with_coord_type(coord_type)
        return _ctype_to_extension_type(ctype)

    def with_edge_type(self, edge_type):
        ctype = self._type.with_edge_type(edge_type)
        return _ctype_to_extension_type(ctype)

    def with_crs(self, crs, crs_type=None):
        if crs_type is None and crs is None:
            ctype = self._type.with_crs(b"", lib.CrsType.NONE)
        elif crs_type is None:
            if not isinstance(crs, bytes):
                crs = crs.encode("UTF-8")
            ctype = self._type.with_crs(crs, lib.CrsType.UNKNOWN)
        else:
            if not isinstance(crs, bytes):
                crs = crs.encode("UTF-8")
            ctype = self._type.with_crs(crs, crs_type)

        return _ctype_to_extension_type(ctype)


class WkbType(VectorType):
    _extension_name = "geoarrow.wkb"

    def wrap_array(self, obj, validate=True):
        return super().wrap_array(obj, validate=validate)


class WktType(VectorType):
    _extension_name = "geoarrow.wkt"

    def wrap_array(self, obj, validate=True):
        return super().wrap_array(obj, validate=validate)


class PointType(VectorType):
    _extension_name = "geoarrow.point"

    def from_geobuffers(self, validity, x, y=None, z_or_m=None, m=None):
        buffers = [
            (0, "uint8", validity),
            (1, "double", x),
            (2, "double", y),
            (3, "double", z_or_m),
            (4, "double", m),
        ]

        return self._from_geobuffers_internal(buffers)


class LinestringType(VectorType):
    _extension_name = "geoarrow.linestring"

    def from_geobuffers(self, validity, coord_offsets, x, y=None, z_or_m=None, m=None):
        buffers = [
            (0, "uint8", validity),
            (1, "int32", coord_offsets),
            (2, "double", x),
            (3, "double", y),
            (4, "double", z_or_m),
            (5, "double", m),
        ]

        return self._from_geobuffers_internal(buffers)


class PolygonType(VectorType):
    _extension_name = "geoarrow.polygon"

    def from_geobuffers(
        self, validity, ring_offsets, coord_offsets, x, y=None, z_or_m=None, m=None
    ):
        buffers = [
            (0, "uint8", validity),
            (1, "int32", ring_offsets),
            (2, "int32", coord_offsets),
            (3, "double", x),
            (4, "double", y),
            (5, "double", z_or_m),
            (6, "double", m),
        ]

        return self._from_geobuffers_internal(buffers)


class MultiPointType(VectorType):
    _extension_name = "geoarrow.multipoint"

    def from_geobuffers(self, validity, coord_offsets, x, y=None, z_or_m=None, m=None):
        buffers = [
            (0, "uint8", validity),
            (1, "int32", coord_offsets),
            (2, "double", x),
            (3, "double", y),
            (4, "double", z_or_m),
            (5, "double", m),
        ]

        return self._from_geobuffers_internal(buffers)


class MultiLinestringType(VectorType):
    _extension_name = "geoarrow.multilinestring"

    def from_geobuffers(
        self,
        validity,
        linestring_offsets,
        coord_offsets,
        x,
        y=None,
        z_or_m=None,
        m=None,
    ):
        buffers = [
            (0, "uint8", validity),
            (1, "int32", linestring_offsets),
            (2, "int32", coord_offsets),
            (3, "double", x),
            (4, "double", y),
            (5, "double", z_or_m),
            (6, "double", m),
        ]

        return self._from_geobuffers_internal(buffers)


class MultiPolygonType(VectorType):
    _extension_name = "geoarrow.multipolygon"

    def from_geobuffers(
        self,
        validity,
        polygon_offsets,
        ring_offsets,
        coord_offsets,
        x,
        y=None,
        z_or_m=None,
        m=None,
    ):
        buffers = [
            (0, "uint8", validity),
            (1, "int32", polygon_offsets),
            (2, "int32", ring_offsets),
            (3, "int32", coord_offsets),
            (4, "double", x),
            (5, "double", y),
            (6, "double", z_or_m),
            (7, "double", m),
        ]

        return self._from_geobuffers_internal(buffers)


def type_cls_from_name(name):
    if name == "geoarrow.wkb":
        return WkbType
    elif name == "geoarrow.wkt":
        return WktType
    elif name == "geoarrow.point":
        return PointType
    elif name == "geoarrow.linestring":
        return LinestringType
    elif name == "geoarrow.polygon":
        return PolygonType
    elif name == "geoarrow.multipoint":
        return MultiPointType
    elif name == "geoarrow.multilinestring":
        return MultiLinestringType
    elif name == "geoarrow.multipolygon":
        return MultiPolygonType
    else:
        raise ValueError(f'Expected valid extension name but got "{name}"')


def _ctype_to_extension_type(ctype):
    cls = type_cls_from_name(ctype.extension_name)
    return cls(ctype)


def _make_default(geometry_type, cls):
    ctype = lib.CVectorType.Make(
        geometry_type, lib.Dimensions.XY, lib.CoordType.SEPARATE
    )
    return cls(ctype)


def wkb() -> WkbType:
    return WkbType.__arrow_ext_deserialize__(pa.binary(), b"")


def large_wkb() -> WkbType:
    return WkbType.__arrow_ext_deserialize__(pa.large_binary(), b"")


def wkt() -> WktType:
    return WktType.__arrow_ext_deserialize__(pa.utf8(), b"")


def large_wkt() -> WktType:
    return WktType.__arrow_ext_deserialize__(pa.large_utf8(), b"")


def point() -> PointType:
    return _make_default(lib.GeometryType.POINT, PointType)


def linestring() -> PointType:
    return _make_default(lib.GeometryType.LINESTRING, LinestringType)


def polygon() -> PolygonType:
    return _make_default(lib.GeometryType.POLYGON, PolygonType)


def multipoint() -> MultiPointType:
    return _make_default(lib.GeometryType.MULTIPOINT, MultiPointType)


def multilinestring() -> MultiLinestringType:
    return _make_default(lib.GeometryType.MULTILINESTRING, MultiLinestringType)


def multipolygon() -> MultiPolygonType:
    return _make_default(lib.GeometryType.MULTIPOLYGON, MultiPolygonType)


def vector_type(
    geometry_type,
    dimensions=lib.Dimensions.XY,
    coord_type=lib.CoordType.SEPARATE,
    edge_type=lib.EdgeType.PLANAR,
    crs=None,
    crs_type=None,
) -> VectorType:
    ctype = lib.CVectorType.Make(geometry_type, dimensions, coord_type)
    cls = type_cls_from_name(ctype.extension_name)
    return cls(ctype).with_edge_type(edge_type).with_crs(crs, crs_type)


_extension_types_registered = False


def register_extension_types(lazy=True):
    global _extension_types_registered

    if lazy and _extension_types_registered is True:
        return

    _extension_types_registered = None

    all_types = [
        wkt(),
        wkb(),
        point(),
        linestring(),
        polygon(),
        multipoint(),
        multilinestring(),
        multipolygon(),
    ]

    n_registered = 0
    for t in all_types:
        try:
            pa.register_extension_type(t)
            n_registered += 1
        except pa.ArrowException:
            pass

    if n_registered != len(all_types):
        raise RuntimeError("Failed to register one or more extension types")

    _extension_types_registered = True


def unregister_extension_types(lazy=True):
    global _extension_types_registered

    if lazy and _extension_types_registered is False:
        return

    _extension_types_registered = None

    all_type_names = [
        "geoarrow.wkb",
        "geoarrow.wkt",
        "geoarrow.point",
        "geoarrow.linestring",
        "geoarrow.polygon",
        "geoarrow.multipoint",
        "geoarrow.multilinestring",
        "geoarrow.multipolygon",
    ]

    n_unregistered = 0
    for t_name in all_type_names:
        try:
            pa.unregister_extension_type(t_name)
            n_unregistered += 1
        except pa.ArrowException:
            pass

    if n_unregistered != len(all_type_names):
        raise RuntimeError("Failed to unregister one or more extension types")

    _extension_types_registered = False
