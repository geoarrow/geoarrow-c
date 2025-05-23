from . import _lib
from ._lib import (
    CKernel,
    SchemaHolder,
    ArrayHolder,
    CGeometryDataType,
    CArrayView,
    CBuilder,
    GeoArrowCException,
)

# Ease the transition between 0.1 and 0.2
CVectorType = CGeometryDataType


class GeometryType:
    """Constants for geometry type. These values are the same as those used
    in well-known binary (i.e, 0-7).

    Examples
    --------

    >>> import geoarrow.c as ga
    >>> ga.GeometryType.MULTIPOINT
    <GeoArrowGeometryType.GEOARROW_GEOMETRY_TYPE_MULTIPOINT: 4>
    """

    #: Unknown or uninitialized geometry type
    GEOMETRY = _lib.GEOARROW_GEOMETRY_TYPE_GEOMETRY
    #: Point geometry type
    POINT = _lib.GEOARROW_GEOMETRY_TYPE_POINT
    #: Linestring geometry type
    LINESTRING = _lib.GEOARROW_GEOMETRY_TYPE_LINESTRING
    #: Polygon geometry type
    POLYGON = _lib.GEOARROW_GEOMETRY_TYPE_POLYGON
    #: Multipoint geometry type
    MULTIPOINT = _lib.GEOARROW_GEOMETRY_TYPE_MULTIPOINT
    #: Multilinestring geometry type
    MULTILINESTRING = _lib.GEOARROW_GEOMETRY_TYPE_MULTILINESTRING
    #: Multipolygon geometry type
    MULTIPOLYGON = _lib.GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON
    #: Geometrycollection geometry type
    GEOMETRYCOLLECTION = _lib.GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION
    #: Box geometry type
    BOX = _lib.GEOARROW_GEOMETRY_TYPE_BOX


class Dimensions:
    """Constants for dimensions.

    Examples
    --------

    >>> import geoarrow.c as ga
    >>> ga.Dimensions.XYZM
    <GeoArrowDimensions.GEOARROW_DIMENSIONS_XYZM: 4>
    """

    #: Unknown or ininitialized dimensions
    UNKNOWN = _lib.GEOARROW_DIMENSIONS_UNKNOWN
    #: XY dimensions
    XY = _lib.GEOARROW_DIMENSIONS_XY
    #: XYZ dimensions
    XYZ = _lib.GEOARROW_DIMENSIONS_XYZ
    #: XYM dimensions
    XYM = _lib.GEOARROW_DIMENSIONS_XYM
    #: XYZM dimensions
    XYZM = _lib.GEOARROW_DIMENSIONS_XYZM


class CoordType:
    """Constants for coordinate type.

    Examples
    --------

    >>> import geoarrow.c as ga
    >>> ga.CoordType.INTERLEAVED
    <GeoArrowCoordType.GEOARROW_COORD_TYPE_INTERLEAVED: 2>
    """

    #: Unknown or uninitialized coordinate type
    UNKNOWN = _lib.GEOARROW_COORD_TYPE_UNKNOWN
    #: Coordinate type composed of separate arrays for each dimension (i.e., a struct)
    SEPARATE = _lib.GEOARROW_COORD_TYPE_SEPARATE
    #: Coordinate type compose of a single array containing all dimensions
    #:(i.e., a fixed-size list)
    INTERLEAVED = _lib.GEOARROW_COORD_TYPE_INTERLEAVED


class EdgeType:
    """Constants for edge type.

    Examples
    --------

    >>> import geoarrow.c as ga
    >>> ga.EdgeType.SPHERICAL
    <GeoArrowEdgeType.GEOARROW_EDGE_TYPE_SPHERICAL: 1>
    """

    #: Edges form a Cartesian line on a plane
    PLANAR = _lib.GEOARROW_EDGE_TYPE_PLANAR
    #: Edges are geodesic on a sphere
    SPHERICAL = _lib.GEOARROW_EDGE_TYPE_SPHERICAL
    #: Edges are geodesic on a spheroid according to the Vincenty algorithm
    VINCENTY = _lib.GEOARROW_EDGE_TYPE_VINCENTY
    #: Edges are geodesic on a spheroid according to the Thomas algorithm
    THOMAS = _lib.GEOARROW_EDGE_TYPE_THOMAS
    #: Edges are geodesic on a spheroid according to the Andoyer algorithm
    ANDOYER = _lib.GEOARROW_EDGE_TYPE_ANDOYER
    #: Edges are geodesic on a spheroid according to the Karney algorithm
    KARNEY = _lib.GEOARROW_EDGE_TYPE_KARNEY


class CrsType:
    """Constants for coordinate reference system type.

    Examples
    --------

    >>> import geoarrow.c as ga
    >>> ga.CrsType.PROJJSON
    <GeoArrowCrsType.GEOARROW_CRS_TYPE_PROJJSON: 2>
    """

    #: Explicit empty coordinate reference system
    NONE = _lib.GEOARROW_CRS_TYPE_NONE
    #: The CRS value is a string of unspecified origin
    UNKNOWN = _lib.GEOARROW_CRS_TYPE_UNKNOWN
    #: The CRS value is a PROJJSON-encoded string
    PROJJSON = _lib.GEOARROW_CRS_TYPE_PROJJSON
    #: The CRS value is a WKT:2019-encoded string
    WKT2_2019 = _lib.GEOARROW_CRS_TYPE_WKT2_2019
    #: The CRS value is a authority:code-encoded string
    AUTHORITY_CODE = _lib.GEOARROW_CRS_TYPE_AUTHORITY_CODE
    #: The CRS value is an opaque producer-defined identifier
    SRID = _lib.GEOARROW_CRS_TYPE_SRID
