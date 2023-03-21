
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

    type_crs = type_obj.with_crs('EPSG:1234', ga.CrsType.UNKNOWN)
    assert type_crs.crs_type == ga.CrsType.UNKNOWN
    assert type_crs.crs == 'EPSG:1234'

def test_constructors():
    assert ga.wkb().extension_name == 'geoarrow.wkb'
    assert ga.large_wkb().extension_name == 'geoarrow.wkb'
    assert ga.wkt().extension_name == 'geoarrow.wkt'
    assert ga.large_wkt().extension_name == 'geoarrow.wkt'
    assert ga.point().extension_name == 'geoarrow.point'
    assert ga.linestring().extension_name == 'geoarrow.linestring'
    assert ga.polygon().extension_name == 'geoarrow.polygon'
    assert ga.multipoint().extension_name == 'geoarrow.multipoint'
    assert ga.multilinestring().extension_name == 'geoarrow.multilinestring'
    assert ga.multipolygon().extension_name == 'geoarrow.multipolygon'

    generic = ga.vector_type(
        ga.GeometryType.POINT,
        ga.Dimensions.XYZ,
        ga.CoordType.INTERLEAVED,
        ga.EdgeType.SPHERICAL,
        'EPSG:1234'
    )
    assert generic.geometry_type == ga.GeometryType.POINT
    assert generic.dimensions == ga.Dimensions.XYZ
    assert generic.coord_type == ga.CoordType.INTERLEAVED
    assert generic.edge_type == ga.EdgeType.SPHERICAL
    assert generic.crs == 'EPSG:1234'
    assert generic.crs_type == ga.CrsType.UNKNOWN
