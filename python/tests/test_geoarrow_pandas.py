import pytest

import pandas as pd
import pyarrow as pa
import geoarrow.pandas as gapd
import geoarrow.pyarrow as ga
import numpy as np


def test_dtype_constructor():
    from_pyarrow = gapd.GeoArrowExtensionDtype(ga.point())
    assert from_pyarrow.name == "geoarrow.point"

    from_ctype = gapd.GeoArrowExtensionDtype(ga.point()._type)
    assert from_ctype.name == "geoarrow.point"

    from_dtype = gapd.GeoArrowExtensionDtype(from_ctype)
    assert from_dtype.name == "geoarrow.point"

    with pytest.raises(TypeError):
        gapd.GeoArrowExtensionDtype(b"1234")


def test_dtype_strings():
    dtype = gapd.GeoArrowExtensionDtype(ga.point())
    assert str(dtype) == "geoarrow.point"

    dtype = gapd.GeoArrowExtensionDtype(ga.point().with_crs("EPSG:1234"))
    assert str(dtype) == 'geoarrow.point[{"crs":"EPSG:1234"}]'


def test_scalar():
    scalar_from_wkt = gapd.GeoArrowExtensionScalar("POINT (0 1)")
    assert scalar_from_wkt.wkt == "POINT (0 1)"
    assert isinstance(scalar_from_wkt.wkb, bytes)
    assert str(scalar_from_wkt) == "POINT (0 1)"
    assert repr(scalar_from_wkt) == 'GeoArrowExtensionScalar("POINT (0 1)")'

    scalar_from_wkb = gapd.GeoArrowExtensionScalar(scalar_from_wkt.wkb)
    assert scalar_from_wkb == scalar_from_wkt

    scalar_from_scalar = gapd.GeoArrowExtensionScalar(scalar_from_wkt)
    assert scalar_from_scalar == scalar_from_wkt

    array = ga.as_geoarrow(["POINT (0 1)", "POINT (1 2)"])
    scalar_from_array0 = gapd.GeoArrowExtensionScalar(array, 0)
    assert scalar_from_array0 == scalar_from_wkt

    scalar_from_array1 = gapd.GeoArrowExtensionScalar(array, 1)
    assert scalar_from_array1 == gapd.GeoArrowExtensionScalar("POINT (1 2)")


def test_array_init_without_type():
    array = gapd.GeoArrowExtensionArray(["POINT (0 1)"])
    assert array._data == ga.array(["POINT (0 1)"])
    assert array._dtype._parent.extension_name == "geoarrow.wkt"


def test_array_init_with_type():
    array = gapd.GeoArrowExtensionArray(["POINT (0 1)"], ga.wkt())
    assert array._data == ga.array(["POINT (0 1)"], ga.wkt())
    assert array._dtype._parent.extension_name == "geoarrow.wkt"


def test_array_basic_methods():
    pa_array = ga.array(["POINT (0 1)", "POINT (1 2)", None])
    array = gapd.GeoArrowExtensionArray(pa_array)

    assert len(array) == 3
    assert all(array[:2] == array[:2])


def test_accessor_parse_all():
    series = pd.Series(["POINT (0 1)"])
    assert series.geoarrow.parse_all() is series
    with pytest.raises(ValueError):
        pd.Series(["NOT WKT"]).geoarrow.parse_all()


def test_accessor_as_wkt():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_wkt()
    assert isinstance(ga_series.dtype.pyarrow_dtype, ga.WktType)


def test_accessor_as_wkb():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_wkb()
    assert isinstance(ga_series.dtype.pyarrow_dtype, ga.WkbType)


def test_accessor_format_wkt():
    with pytest.raises(TypeError):
        pd.Series(["POINT (0 1)"]).geoarrow.format_wkt()

    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_geoarrow().geoarrow.format_wkt()
    assert ga_series.dtype.pyarrow_dtype == pa.utf8()


def test_accessor_format_wkb():
    with pytest.raises(TypeError):
        pd.Series(["POINT (0 1)"]).geoarrow.format_wkb()

    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_geoarrow().geoarrow.format_wkb()
    assert ga_series.dtype.pyarrow_dtype == pa.binary()

    # Currently handles ChunkedArray explicitly
    chunked = pa.chunked_array([ga.array(["POINT (0 1)"])])
    ga_series = pd.Series(
        chunked, dtype=pd.ArrowDtype(chunked.type)
    ).geoarrow.format_wkb()
    assert ga_series.dtype.pyarrow_dtype == pa.binary()


def test_accessor_as_geoarrow():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_geoarrow()
    assert isinstance(ga_series.dtype.pyarrow_dtype, ga.PointType)


def test_accessor_bounds():
    df = pd.Series(["POINT (0 1)"]).geoarrow.bounds()
    assert isinstance(df, pd.DataFrame)
    assert df.xmin[0] == 0
    assert df.ymin[0] == 1
    assert df.xmax[0] == 0
    assert df.ymax[0] == 1


def test_accessor_total_bounds():
    df = pd.Series(["POINT (0 1)"]).geoarrow.total_bounds()
    assert isinstance(df, pd.DataFrame)
    assert df.xmin[0] == 0
    assert df.ymin[0] == 1
    assert df.xmax[0] == 0
    assert df.ymax[0] == 1


def test_accessor_point_coords():
    pass


def test_accessor_with_coord_type():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_coord_type(
        ga.CoordType.INTERLEAVED
    )
    assert ga_series.dtype.pyarrow_dtype.coord_type == ga.CoordType.INTERLEAVED


def test_accessor_with_edge_type():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_edge_type(
        ga.EdgeType.SPHERICAL
    )
    assert ga_series.dtype.pyarrow_dtype.edge_type == ga.EdgeType.SPHERICAL


def test_accessor_with_crs():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_crs("EPSG:1234")
    assert ga_series.dtype.pyarrow_dtype.crs == "EPSG:1234"


def test_accessor_with_dimensions():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_dimensions(ga.Dimensions.XYZ)
    assert ga_series.dtype.pyarrow_dtype.dimensions == ga.Dimensions.XYZ
