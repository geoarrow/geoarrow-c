"""
Contains pyarrow integration for the geoarrow Python bindings.

Examples
--------

>>> import geoarrow.pyarrow as ga
"""

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

from ._kernel import Kernel

from ._array import array

from . import _scalar

from ._compute import (
    parse_all,
    as_wkt,
    as_wkb,
    infer_type_common,
    as_geoarrow,
    format_wkt,
    unique_geometry_types,
    box,
    box_agg
)

register_extension_types()
