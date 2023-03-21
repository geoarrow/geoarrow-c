
import pyarrow as pa

from . import lib
from .lib import GeometryType, Dimensions, CoordType, EdgeType, CrsType

class VectorType(pa.ExtensionType):
    _extension_name = None

    def __init__(self, c_vector_type):
        if not isinstance(c_vector_type, lib.CVectorType):
            raise TypeError('geoarrow.pyarrow.VectorType must be created from a CVectorType')
        self._type = c_vector_type
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
    cls(ctype)
