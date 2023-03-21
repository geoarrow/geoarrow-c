
import pyarrow as pa
import pytest

import geoarrow.lib as lib
import geoarrow.pyarrow as ga

def test_vector_type_basic():
    ctype = lib.CVectorType.Make(
        ga.GeometryType.POINT,
        ga.Dimensions.XY,
        ga.CoordType.SEPARATE
    )

    pa_type = ga.PointType(ctype)

    assert pa_type.geometry_type == ga.GeometryType.POINT
    assert pa_type.dimensions == ga.Dimensions.XY
    assert pa_type.coord_type == ga.CoordType.SEPARATE

    expected_storage = pa.struct(
        [pa.field('x', pa.float64()), pa.field('y', pa.float64())]
    )
    assert pa_type.storage_type == expected_storage

    with pytest.raises(ValueError):
        ga.LinestringType(ctype)

def test_vector_type_with():
    ctype = lib.CVectorType.Make(
        ga.GeometryType.POINT,
        ga.Dimensions.XY,
        ga.CoordType.SEPARATE
    )

    type_obj = ga.PointType(ctype)

    type_linestring = type_obj.with_geometry_type(ga.GeometryType.LINESTRING)
    assert type_linestring.geometry_type == ga.GeometryType.LINESTRING

    type_xyz = type_obj.with_dimensions(ga.Dimensions.XYZ)
    assert type_xyz.dimensions == ga.Dimensions.XYZ

    type_interleaved = type_obj.with_coord_type(ga.CoordType.INTERLEAVED)
    assert type_interleaved.coord_type == ga.CoordType.INTERLEAVED

    type_spherical = type_obj.with_edge_type(ga.EdgeType.SPHERICAL)
    assert type_spherical.edge_type == ga.EdgeType.SPHERICAL

    type_crs = type_obj.with_crs(b'EPSG:1234', ga.CrsType.UNKNOWN)
    assert type_crs.crs_type == ga.CrsType.UNKNOWN
    assert type_crs.crs == b'EPSG:1234'
