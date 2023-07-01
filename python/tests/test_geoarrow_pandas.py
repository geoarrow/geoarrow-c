import pytest

import pandas as pd
import pyarrow as pa
import geoarrow.pandas
import geoarrow.pyarrow as ga


def test_parse_all():
    series = pd.Series(["POINT (0 1)"])
    assert series.geoarrow.parse_all() is series
    with pytest.raises(ValueError):
        pd.Series(["NOT WKT"]).geoarrow.parse_all()


def test_as_wkt():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_wkt()
    assert isinstance(ga_series.dtype.pyarrow_dtype, ga.WktType)


def test_as_wkb():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_wkb()
    assert isinstance(ga_series.dtype.pyarrow_dtype, ga.WkbType)


def test_format_wkt():
    with pytest.raises(TypeError):
        pd.Series(["POINT (0 1)"]).geoarrow.format_wkt()

    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_geoarrow().geoarrow.format_wkt()
    assert ga_series.dtype.pyarrow_dtype == pa.utf8()


def test_format_wkb():
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


def test_as_geoarrow():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.as_geoarrow()
    assert isinstance(ga_series.dtype.pyarrow_dtype, ga.PointType)


def test_bounds():
    df = pd.Series(["POINT (0 1)"]).geoarrow.bounds()
    assert isinstance(df, pd.DataFrame)
    assert df.xmin[0] == 0
    assert df.ymin[0] == 1
    assert df.xmax[0] == 0
    assert df.ymax[0] == 1


def test_total_bounds():
    df = pd.Series(["POINT (0 1)"]).geoarrow.total_bounds()
    assert isinstance(df, pd.DataFrame)
    assert df.xmin[0] == 0
    assert df.ymin[0] == 1
    assert df.xmax[0] == 0
    assert df.ymax[0] == 1


def test_point_coords():
    pass

def test_with_coord_type():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_coord_type(ga.CoordType.INTERLEAVED)
    assert ga_series.dtype.pyarrow_dtype.coord_type == ga.CoordType.INTERLEAVED


def test_with_edge_type():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_edge_type(ga.EdgeType.SPHERICAL)
    assert ga_series.dtype.pyarrow_dtype.edge_type == ga.EdgeType.SPHERICAL


def test_with_crs():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_crs("EPSG:1234")
    assert ga_series.dtype.pyarrow_dtype.crs == "EPSG:1234"


def test_with_dimensions():
    ga_series = pd.Series(["POINT (0 1)"]).geoarrow.with_dimensions(ga.Dimensions.XYZ)
    assert ga_series.dtype.pyarrow_dtype.dimensions == ga.Dimensions.XYZ
