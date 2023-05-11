from ..lib import GeometryType, Dimensions, CoordType, EdgeType, CrsType

from ._type import (
    VectorType,
    wkb,
    large_wkb,
    wkt,
    large_wkt,
    point,
    linestring,
    polygon,
    multipoint,
    multilinestring,
    multipolygon,
    vector_type,
    register_extension_types,
    unregister_extension_types,
)

from ._compute import Kernel

from ._array import array

from . import _scalar

register_extension_types()
