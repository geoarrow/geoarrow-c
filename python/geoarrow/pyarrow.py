
import sys

import pyarrow as pa

from . import lib
from .lib import GeometryType, Dimensions, CoordType, EdgeType, CrsType


class VectorScalar(pa.ExtensionScalar):
    pass


class PointScalar(VectorScalar):
    pass


class LinestringScalar(VectorScalar):
    pass


class PolygonScalar(VectorScalar):
    pass


class MultiPointScalar(VectorScalar):
    pass


class MultiLinestringScalar(VectorScalar):
    pass


class MultiPolygonScalar(VectorScalar):
    pass


class VectorArray(pa.ExtensionArray):

    def as_wkt(self):
        if self.type.extension_name == 'geoarrow.wkt':
            return self
        kernel = Kernel.as_wkt(self.type)
        return kernel.push(self)

    def as_wkb(self):
        if self.type.extension_name == 'geoarrow.wkb':
            return self
        kernel = Kernel.as_wkb(self.type)
        return kernel.push(self)

    def as_geoarrow(self, type_to=None):
        if type_to is None:
            raise NotImplementedError('Auto-detection of best geoarrow type')

        if isinstance(type_to, WktType):
            return self.as_wkt()
        elif isinstance(type_to, WkbType):
            return self.as_wkb()
        elif not isinstance(type_to, VectorType):
            raise TypeError('type_to must inherit from VectorType')

        if self.type._type.id == type_to._type.id:
            return self

        kernel = Kernel.as_geoarrow(self.type, type_to._type.id)
        return kernel.push(self)

    def __repr__(self):
        n_values_to_show = 10
        max_width = 70

        if len(self) > n_values_to_show:
            n_extra = len(self) - n_values_to_show
            value_s = 'values' if n_extra != 1 else 'value'
            head = self[:int(n_values_to_show / 2)]
            mid = f'...{n_extra} {value_s}...'
            tail = self[int(-n_values_to_show / 2):]
        else:
            head = self
            mid = ''
            tail = self[:0]

        try:
            kernel = Kernel.as_wkt(self.type)
            head = kernel.push(head).storage
            tail = kernel.push(tail).storage
        except Exception as e:
            err = f'* 1 or more display values failed to parse\n* {str(e)}'
            type_name = type(self).__name__
            super_repr = super().__repr__()
            return f'{type_name}:{repr(self.type)}[{len(self)}]\n{err}\n{super_repr}'


        head_str = [f'<{item.as_py()}>' for item in head]
        tail_str = [f'<{item.as_py()}>' for item in tail]
        for i in range(len(head)):
            if len(head_str[i]) > max_width:
                head_str[i] = f'{head_str[i][:(max_width - 4)]}...>'
        for i in range(len(tail)):
            if len(tail_str[i]) > max_width:
                tail_str[i] = f'{tail_str[i][:(max_width - 4)]}...>'

        type_name = type(self).__name__
        head_str = '\n'.join(head_str)
        tail_str = '\n'.join(tail_str)
        items_str = f'{head_str}\n{mid}\n{tail_str}'

        return f'{type_name}:{repr(self.type)}[{len(self)}]\n{items_str}'


class PointArray(VectorArray):
    pass


class LinestringArray(VectorArray):
    pass


class PolygonArray(VectorArray):
    pass


class MultiPointArray(VectorArray):
    pass


class MultiLinestringArray(VectorArray):
    pass


class MultiPolygonArray(VectorArray):
    pass


class VectorType(pa.ExtensionType):
    _extension_name = None

    def __init__(self, c_vector_type):
        if not isinstance(c_vector_type, lib.CVectorType):
            raise TypeError('geoarrow.pyarrow.VectorType must be created from a CVectorType')
        self._type = c_vector_type
        if self._type.extension_name != type(self)._extension_name:
            raise ValueError(
                f'Expected CVectorType with extension name "{type(self)._extension_name}" but got "{self._type.extension_name}"')

        storage_schema = self._type.to_storage_schema()
        storage_type = pa.DataType._import_from_c(storage_schema._addr())
        pa.ExtensionType.__init__(self, storage_type, self._type.extension_name)

    def __repr__(self):
        return f'{type(self).__name__}({repr(self._type)})'

    def __arrow_ext_serialize__(self):
        return self._type.extension_metadata

    @staticmethod
    def _import_from_c(addr):
        field = pa.Field._import_from_c(addr)
        if not field.metadata or 'ARROW:extension:name' not in field.metadata:
            return field.type

        schema = lib.SchemaHolder()
        field._export_to_c(schema._addr())

        c_vector_type = lib.CVectorType.FromExtension(schema)
        cls = _type_cls_from_name(c_vector_type.extension_name.decode('UTF-8'))
        cls(c_vector_type)

    @classmethod
    def __arrow_ext_deserialize__(cls, storage_type, serialized):
        schema = lib.SchemaHolder()
        storage_type._export_to_c(schema._addr())

        c_vector_type = lib.CVectorType.FromStorage(
            schema,
            cls._extension_name.encode('UTF-8'),
            serialized
        )

        return cls(c_vector_type)

    def __arrow_ext_class__(self):
        return _array_cls_from_name(self.extension_name)

    def __arrow_ext_scalar_class__(self):
        return _scalar_cls_from_name(self.extension_name)

    def wrap_array(self, obj, validate=False):
        out = super().wrap_array(obj)
        if validate:
            validator = Kernel.visit_void_agg(self)
            validator.push(out)
            validator.finish()

        return out

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
        return self._type.crs.decode('UTF-8')

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
            ctype = self._type.with_crs(b'', CrsType.NONE)
        elif crs_type is None:
            if not isinstance(crs, bytes):
                crs = crs.encode('UTF-8')
            ctype = self._type.with_crs(crs, CrsType.UNKNOWN)
        else:
            if not isinstance(crs, bytes):
                crs = crs.encode('UTF-8')
            ctype = self._type.with_crs(crs, crs_type)

        return _ctype_to_extension_type(ctype)


class WkbType(VectorType):
    _extension_name = 'geoarrow.wkb'

    def wrap_array(self, obj, validate=True):
        return super().wrap_array(obj, validate=validate)


class WktType(VectorType):
    _extension_name = 'geoarrow.wkt'

    def wrap_array(self, obj, validate=True):
        return super().wrap_array(obj, validate=validate)


class PointType(VectorType):
    _extension_name = 'geoarrow.point'


class LinestringType(VectorType):
    _extension_name = 'geoarrow.linestring'


class PolygonType(VectorType):
    _extension_name = 'geoarrow.polygon'


class MultiPointType(VectorType):
    _extension_name = 'geoarrow.multipoint'


class MultiLinestringType(VectorType):
    _extension_name = 'geoarrow.multilinestring'


class MultiPolygonType(VectorType):
    _extension_name = 'geoarrow.multipolygon'


def _type_cls_from_name(name):
    if name == 'geoarrow.wkb':
        return WkbType
    elif name == 'geoarrow.wkt':
        return WktType
    elif name == 'geoarrow.point':
        return PointType
    elif name == 'geoarrow.linestring':
        return LinestringType
    elif name == 'geoarrow.polygon':
        return PolygonType
    elif name == 'geoarrow.multipoint':
        return MultiPointType
    elif name == 'geoarrow.multilinestring':
        return MultiLinestringType
    elif name == 'geoarrow.multipolygon':
        return MultiPolygonType
    else:
        raise ValueError(f'Expected valid extension name but got "{name}"')

def _array_cls_from_name(name):
    if name == 'geoarrow.wkb':
        return VectorArray
    elif name == 'geoarrow.wkt':
        return VectorArray
    elif name == 'geoarrow.point':
        return PointArray
    elif name == 'geoarrow.linestring':
        return LinestringArray
    elif name == 'geoarrow.polygon':
        return PolygonArray
    elif name == 'geoarrow.multipoint':
        return MultiPointArray
    elif name == 'geoarrow.multilinestring':
        return MultiLinestringArray
    elif name == 'geoarrow.multipolygon':
        return MultiPolygonArray
    else:
        raise ValueError(f'Expected valid extension name but got "{name}"')


def _scalar_cls_from_name(name):
    if name == 'geoarrow.wkb':
        return VectorScalar
    elif name == 'geoarrow.wkt':
        return VectorScalar
    elif name == 'geoarrow.point':
        return PointScalar
    elif name == 'geoarrow.linestring':
        return LinestringScalar
    elif name == 'geoarrow.polygon':
        return PolygonScalar
    elif name == 'geoarrow.multipoint':
        return MultiPointScalar
    elif name == 'geoarrow.multilinestring':
        return MultiLinestringScalar
    elif name == 'geoarrow.multipolygon':
        return MultiPolygonScalar
    else:
        raise ValueError(f'Expected valid extension name but got "{name}"')


def _ctype_to_extension_type(ctype):
    cls = _type_cls_from_name(ctype.extension_name)
    return cls(ctype)


def _make_default(geometry_type, cls):
    ctype = lib.CVectorType.Make(geometry_type, Dimensions.XY, CoordType.SEPARATE)
    return cls(ctype)


def wkb() -> WkbType:
    return WkbType.__arrow_ext_deserialize__(pa.binary(), b'')


def large_wkb() ->WkbType:
    return WkbType.__arrow_ext_deserialize__(pa.large_binary(), b'')


def wkt() -> WktType:
    return WktType.__arrow_ext_deserialize__(pa.utf8(), b'')


def large_wkt() -> WktType:
    return WktType.__arrow_ext_deserialize__(pa.large_utf8(), b'')


def point() -> PointType:
    return _make_default(GeometryType.POINT, PointType)


def linestring() -> PointType:
    return _make_default(GeometryType.LINESTRING, LinestringType)


def polygon() -> PolygonType:
    return _make_default(GeometryType.POLYGON, PolygonType)


def multipoint() -> MultiPointType:
    return _make_default(GeometryType.MULTIPOINT, MultiPointType)


def multilinestring() -> MultiLinestringType:
    return _make_default(GeometryType.MULTILINESTRING, MultiLinestringType)

def multipolygon() -> MultiPolygonType:
    return _make_default(GeometryType.MULTIPOLYGON, MultiPolygonType)


def vector_type(geometry_type,
                dimensions=Dimensions.XY,
                coord_type=CoordType.SEPARATE,
                edge_type=EdgeType.PLANAR,
                crs=None, crs_type=None) -> VectorType:
    ctype = lib.CVectorType.Make(geometry_type, dimensions, coord_type)
    cls = _type_cls_from_name(ctype.extension_name)
    return cls(ctype).with_edge_type(edge_type).with_crs(crs, crs_type)


def array(obj, type=None, *args, validate=True, **kwargs) -> VectorArray:
    if type is None:
        arr = pa.array(obj, *args, **kwargs)

        if arr.type == pa.utf8():
            return wkt().wrap_array(arr, validate=validate)
        elif arr.type == pa.large_utf8():
            return large_wkt().wrap_array(arr, validate=validate)
        elif arr.type == pa.binary():
            return wkb().wrap_array(arr, validate=validate)
        else:
            raise TypeError(f"Can't create geoarrow.array from Arrow array of type {type}")

    type_is_geoarrow = isinstance(type, VectorType)
    type_is_wkb_or_wkt = type.extension_name in ('geoarrow.wkt', 'geoarrow.wkb')

    if type_is_geoarrow and type_is_wkb_or_wkt:
        arr = pa.array(obj, type.storage_type, *args, **kwargs)
        return type.wrap_array(arr, validate=validate)

    # Eventually we will be able to handle more types (e.g., parse wkt or wkb
    # into a geoarrow type)
    raise TypeError(f"Can't create geoarrow.array for type {type}")

class Kernel:

    def __init__(self, name, type_in, **kwargs) -> None:
        if not isinstance(type_in, pa.DataType):
            raise TypeError('Expected `type_in` to inherit from pyarrow.DataType')

        self._name = str(name)
        self._kernel = lib.CKernel(self._name.encode('UTF-8'))
        # True for all the kernels that currently exist
        self._is_agg = self._name.endswith('_agg')

        type_in_schema = lib.SchemaHolder()
        type_in._export_to_c(type_in_schema._addr())

        options = Kernel._pack_options(kwargs)

        type_out_schema = self._kernel.start(type_in_schema, options)
        self._type_out = VectorType._import_from_c(type_out_schema._addr())
        self._type_in = type_in

    def push(self, arr):
        if isinstance(arr, pa.ChunkedArray):
            chunks_out = []
            for chunk_in in arr:
                chunks_out.append(self.push(chunk_in))
            return pa.chunked_array(chunks_out)
        elif not isinstance(arr, pa.Array):
            raise TypeError(f'Expected pyarrow.Array or pyarrow.ChunkedArray but got {type(arr)}')

        array_in = lib.ArrayHolder()
        arr._export_to_c(array_in._addr())

        if self._is_agg:
            self._kernel.push_batch_agg(array_in)
        else:
            array_out = self._kernel.push_batch(array_in)
            return pa.Array._import_from_c(array_out._addr(), self._type_out)

    def finish(self):
        if self._is_agg:
            out = self._kernel.finish_agg()
            return pa.Array._import_from_c(out._addr(), self._type_out)
        else:
            self._kernel.finish()

    @staticmethod
    def void(type_in):
        return Kernel('void', type_in)

    @staticmethod
    def void_agg(type_in):
        return Kernel('void_agg', type_in)

    @staticmethod
    def visit_void_agg(type_in):
        return Kernel('visit_void_agg', type_in)

    @staticmethod
    def as_wkt(type_in):
        return Kernel('as_wkt', type_in)

    @staticmethod
    def as_wkb(type_in):
        return Kernel('as_wkb', type_in)

    @staticmethod
    def as_geoarrow(type_in, type_id):
        return Kernel('as_geoarrow', type_in, type=type_id)

    @staticmethod
    def _pack_options(options):
        if not options:
            return b''

        bytes = len(options).to_bytes(4, sys.byteorder, signed=True)
        for k, v in options.items():
            k = str(k)
            bytes += len(k).to_bytes(4, sys.byteorder, signed=True)
            bytes += k.encode('UTF-8')

            v = str(v)
            bytes += len(v).to_bytes(4, sys.byteorder, signed=True)
            bytes += v.encode('UTF-8')

        return bytes


_extension_types_registered = False

def register_extension_types(lazy=True):
    global _extension_types_registered

    if lazy and _extension_types_registered is True:
        return

    _extension_types_registered = None

    all_types = [
        wkt(), wkb(),
        point(), linestring(), polygon(),
        multipoint(), multilinestring(), multipolygon()
    ]

    n_registered = 0
    for t in all_types:
        try:
            pa.register_extension_type(t)
            n_registered += 1
        except pa.ArrowException:
            pass

    if n_registered != len(all_types):
        raise RuntimeError('Failed to register one or more extension types')

    _extension_types_registered = True


def unregister_extension_types(lazy=True):
    global _extension_types_registered

    if lazy and _extension_types_registered is False:
        return

    _extension_types_registered = None

    all_type_names = [
        'geoarrow.wkb', 'geoarrow.wkt',
        'geoarrow.point', 'geoarrow.linestring', 'geoarrow.polygon',
        'geoarrow.multipoint', 'geoarrow.multilinestring', 'geoarrow.multipolygon'
    ]

    n_unregistered = 0
    for t_name in all_type_names:
        try:
            pa.unregister_extension_type(t_name)
            n_unregistered += 1
        except pa.ArrowException:
            pass

    if n_unregistered != len(all_type_names):
        raise RuntimeError('Failed to unregister one or more extension types')

    _extension_types_registered = False

# Do it!
register_extension_types()
