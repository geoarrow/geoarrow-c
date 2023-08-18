from tempfile import TemporaryDirectory

import pyarrow as pa
import pyarrow.dataset as ds
import pyarrow.parquet as pq
import pytest

import geoarrow.pyarrow as ga
import geoarrow.dataset as gads


def test_geodataset_in_memory():
    table1 = pa.table([ga.array(["POINT (0.5 1.5)"])], ["geometry"])
    table2 = pa.table([ga.array(["POINT (2.5 3.5)"])], ["geometry"])

    geods = gads.dataset([table1, table2])
    assert isinstance(geods._parent, ds.InMemoryDataset)
    assert len(list(geods._parent.get_fragments())) == 2

    filtered1 = geods.filter_fragments("POLYGON ((0 1, 1 1, 1 2, 0 2, 0 1))").to_table()
    assert filtered1.num_rows == 1

    with pytest.raises(TypeError):
        gads.dataset([table1], use_row_groups=True)


def test_geodataset_parquet():
    table1 = pa.table([ga.array(["POINT (0.5 1.5)"])], ["geometry"])
    table2 = pa.table([ga.array(["POINT (2.5 3.5)"])], ["geometry"])
    with TemporaryDirectory() as td:
        pq.write_table(table1, f"{td}/table1.parquet")
        pq.write_table(table2, f"{td}/table2.parquet")
        geods = gads.dataset(
            [f"{td}/table1.parquet", f"{td}/table2.parquet"], use_row_groups=False
        )

        filtered1 = geods.filter_fragments(
            "POLYGON ((0 1, 1 1, 1 2, 0 2, 0 1))"
        ).to_table()
        assert filtered1.num_rows == 1


def test_geodataset_parquet_rowgroups():
    table = pa.table([ga.array(["POINT (0.5 1.5)", "POINT (2.5 3.5)"])], ["geometry"])
    with TemporaryDirectory() as td:
        pq.write_table(table, f"{td}/table.parquet", row_group_size=1)

        geods = gads.dataset(f"{td}/table.parquet")
        assert len(geods.get_fragments()) == 2

        filtered1 = geods.filter_fragments(
            "POLYGON ((0 1, 1 1, 1 2, 0 2, 0 1))"
        ).to_table()
        assert filtered1.num_rows == 1


def test_geodataset_parquet_index_rowgroups():
    array_wkt = ga.array(
        ["LINESTRING (0.5 1.5, 2.5 3.5)", "LINESTRING (4.5 5.5, 6.5 7.5)"]
    )
    array_geoarrow = ga.as_geoarrow(
        ["LINESTRING (8.5 9.5, 10.5 11.5)", "LINESTRING (12.5 13.5, 14.5 15.5)"]
    )

    table_wkt = pa.table([array_wkt], ["geometry"])
    table_geoarrow = pa.table([array_geoarrow], ["geometry"])
    table_both = pa.table(
        [array_wkt, array_geoarrow], ["geometry_wkt", "geometry_geoarrow"]
    )

    with TemporaryDirectory() as td:
        pq.write_table(table_wkt, f"{td}/table_wkt.parquet", row_group_size=1)
        pq.write_table(table_geoarrow, f"{td}/table_geoarrow.parquet", row_group_size=1)
        pq.write_table(
            table_geoarrow,
            f"{td}/table_geoarrow_nostats.parquet",
            row_group_size=1,
            write_statistics=False,
        )
        pq.write_table(table_both, f"{td}/table_both.parquet", row_group_size=1)

        ds_wkt = gads.dataset(f"{td}/table_wkt.parquet")
        ds_geoarrow = gads.dataset(f"{td}/table_geoarrow.parquet")
        ds_geoarrow_nostats = gads.dataset(f"{td}/table_geoarrow_nostats.parquet")
        ds_both = gads.dataset(f"{td}/table_both.parquet")

        index_wkt = ds_wkt.index_fragments()
        index_geoarrow = ds_geoarrow.index_fragments()
        index_geoarrow_nostats = ds_geoarrow_nostats.index_fragments()
        index_both = ds_both.index_fragments()

        # All the fragment indices should be the same
        assert index_geoarrow.column(0) == index_wkt.column(0)
        assert index_geoarrow_nostats.column(0) == index_wkt.column(0)
        assert index_both.column(0) == index_wkt.column(0)

        # The wkt index should be the same in index_both and index_wkt
        assert index_both.column("geometry_wkt") == index_wkt.column("geometry")

        # The geoarrow index should be the same everywhere
        assert index_geoarrow_nostats.column("geometry") == index_geoarrow.column(
            "geometry"
        )
        assert index_both.column("geometry_geoarrow") == index_geoarrow.column(
            "geometry"
        )


def test_geodataset_parquet_filter_rowgroups_with_stats():
    arr = ga.as_geoarrow(["POINT (0.5 1.5)", "POINT (2.5 3.5)"])
    table = pa.table([arr], ["geometry"])
    with TemporaryDirectory() as td:
        pq.write_table(table, f"{td}/table.parquet", row_group_size=1)

        geods = gads.dataset(f"{td}/table.parquet")
        assert len(geods.get_fragments()) == 2

        geods._build_index_using_stats(["geometry"])

        filtered1 = geods.filter_fragments(
            "POLYGON ((0 1, 1 1, 1 2, 0 2, 0 1))"
        ).to_table()
        assert filtered1.num_rows == 1
