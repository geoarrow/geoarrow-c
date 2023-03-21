
import pyarrow as pa

from . import lib
from .lib import GeometryType, Dimensions, CoordType, EdgeType, CrsType

class VectorType(pa.ExtensionType):
    _extension_name = None

    def __init__(self, c_vector_type):
        if not isinstance(c_vector_type, lib.CVectorType):
            raise TypeError('geoarrow.pyarrow.VectorType must be created from a CVectorType')
        self._type = c_vector_type
        if self._type.extension_name != type(self)._extension_name:
            raise ValueError(
                f'Expected CVectorType with extension name "{type(self)._extension_name}" but got "{self._type.extension_name}"')

        storage_schema = self._type.to_schema()
        storage_type = pa.DataType._import_from_c(storage_schema._addr())
        pa.ExtensionType.__init__(self, storage_type, self._type.extension_name)

    def __arrow_ext_serialize__(self):
        return self._type.extension_metadata

    @classmethod
    def __arrow_ext_deserialize__(cls, storage_type, serialized):
        schema = lib.SchemaHolder()
        storage_type._export_to_c(schema._addr())

        c_vector_type = lib.CVectorType.FromStorage(schema, cls._extension_name, serialized)
        return cls(c_vector_type)

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
        return self._type.crs

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
            ctype = self._type.with_crs(crs, CrsType.UNKNOWN)
        else:
            ctype = self._type.with_crs(crs, crs_type)

        return _ctype_to_extension_type(ctype)


class WkbType(VectorType):
    _extension_name = 'geoarrow.wkb'


class WktType(VectorType):
    _extension_name = 'geoarrow.wkt'


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

def _ctype_to_extension_type(ctype):
    cls = _type_cls_from_name(ctype.extension_name)
    return cls(ctype)
