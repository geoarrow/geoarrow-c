from concurrent.futures import ThreadPoolExecutor, wait

import pyarrow as _pa
import pyarrow.types as _types
import pyarrow.dataset as _ds
import pyarrow.compute as _compute
import pyarrow.parquet as _pq
from . import pyarrow as _ga
from .pyarrow._kernel import Kernel


def dataset(*args, geometry_columns=None, use_row_groups=None, **kwargs):
    parent = _ds.dataset(*args, **kwargs)

    if use_row_groups is None:
        use_row_groups = isinstance(parent, _ds.FileSystemDataset) and isinstance(
            parent.format, _ds.ParquetFileFormat
        )
    if use_row_groups:
        return ParquetRowGroupGeoDataset(parent, geometry_columns=geometry_columns)
    else:
        return GeoDataset(parent, geometry_columns=geometry_columns)


class GeoDataset:
    def __init__(self, parent, geometry_columns=None):
        self._index = None
        self._geometry_columns = geometry_columns
        self._geometry_types = None
        self._fragments = None

        if not isinstance(parent, _ds.Dataset):
            raise TypeError("parent must be a pyarrow.dataset.Dataset")
        self._parent = parent

    @property
    def schema(self):
        return self._parent.schema

    def get_fragments(self):
        if self._fragments is None:
            self._fragments = tuple(self._parent.get_fragments())

        return self._fragments

    @property
    def geometry_columns(self):
        if self._geometry_columns is None:
            schema = self.schema
            geometry_columns = []
            for name, type in zip(schema.names, schema.types):
                if isinstance(type, _ga.VectorType):
                    geometry_columns.append(name)
            self._geometry_columns = tuple(geometry_columns)

        return self._geometry_columns

    @property
    def geometry_types(self):
        if self._geometry_types is None:
            geometry_types = []
            for col in self.geometry_columns:
                type = self.schema.field(col).type
                if isinstance(type, _ga.VectorType):
                    geometry_types.append(type)
                elif _types.is_binary(type):
                    geometry_types.append(_ga.wkb())
                elif _types.is_string(type):
                    geometry_types.append(_ga.wkt())
                else:
                    raise TypeError(f"Unsupported type for geometry column: {type}")

            self._geometry_types = tuple(geometry_types)

        return self._geometry_types

    def index_fragments(self, num_threads=None):
        if self._index is None:
            self._index = self._build_index(self.geometry_columns, num_threads)

        return self._index

    def _build_index(self, geometry_columns, num_threads=None):
        return GeoDataset._index_fragments(
            self.get_fragments(), geometry_columns, num_threads=num_threads
        )

    def filter_fragments(self, target):
        if isinstance(target, str):
            target = [target]
        target_box = _ga.box_agg(target)
        maybe_intersects = GeoDataset._index_box_intersects(
            self.index_fragments(), target_box, self.geometry_columns
        )
        fragment_indices = [scalar.as_py() for scalar in maybe_intersects]
        fragments = self.get_fragments()
        fragments_filtered = [fragments[i] for i in fragment_indices]

        if isinstance(self._parent, _ds.FileSystemDataset):
            return _ds.FileSystemDataset(
                fragments_filtered, self.schema, self._parent.format
            )
        else:
            tables = [fragment.to_table() for fragment in fragments_filtered]
            return _ds.InMemoryDataset(tables)

    @staticmethod
    def _index_fragment(fragment, column):
        scanner = fragment.scanner(columns=[column])
        reader = scanner.to_reader()
        kernel = Kernel.box_agg(reader.schema.types[0])
        for batch in reader:
            kernel.push(batch.column(0))
        return kernel.finish()

    @staticmethod
    def _index_fragments(fragments, columns, num_threads=None):
        columns = list(columns)
        if num_threads is None:
            num_threads = _pa.cpu_count()

        num_fragments = len(fragments)
        metadata = [_pa.array(range(num_fragments))]
        if not columns:
            return _pa.table(metadata, names=["_fragment_index"])

        with ThreadPoolExecutor(max_workers=num_threads) as executor:
            futures = []
            for column in columns:
                for fragment in fragments:
                    future = executor.submit(
                        GeoDataset._index_fragment, fragment, column
                    )
                    futures.append(future)

            wait(futures)

            results = []
            for i, column in enumerate(columns):
                results.append(
                    [
                        futures[i * num_fragments + j].result()
                        for j in range(num_fragments)
                    ]
                )

            result_arrays = [_pa.concat_arrays(result) for result in results]
            return _pa.table(
                metadata + result_arrays, names=["_fragment_index"] + columns
            )

    @staticmethod
    def _index_box_intersects(index, box, columns):
        xmin, xmax, ymin, ymax = box.as_py().values()
        expressions = []
        for col in columns:
            expr = (
                (_compute.field(col, "xmin") <= xmax)
                & (_compute.field(col, "xmax") >= xmin)
                & (_compute.field(col, "ymin") <= ymax)
                & (_compute.field(col, "ymax") >= ymin)
            )
            expressions.append(expr)

        expr = expressions[0]
        for i in range(1, len(expressions)):
            expr = expr | expressions[i]

        result = _ds.dataset(index).filter(expr).to_table()
        return result.column(0)


class ParquetRowGroupGeoDataset(GeoDataset):
    def __init__(self, parent, geometry_columns=None, use_column_statistics=True):
        if not isinstance(parent, _ds.FileSystemDataset) or not isinstance(
            parent.format, _ds.ParquetFileFormat
        ):
            raise TypeError(
                "ParquetRowGroupGeoDataset() is only supported for Parquet datasets"
            )

        row_group_fragments = []
        row_group_ids = []

        for file_fragment in parent.get_fragments():
            for i, row_group_fragment in enumerate(file_fragment.split_by_row_group()):
                row_group_fragments.append(row_group_fragment)
                # Keep track of the row group IDs so we can possibly accellerate
                # building an index later where column statistics are supported
                row_group_ids.append(i)

        super().__init__(
            _ds.FileSystemDataset(row_group_fragments, parent.schema, parent.format),
            geometry_columns=geometry_columns,
        )
        self._fragments = row_group_fragments
        self._row_group_ids = row_group_ids
        self._use_column_statistics = use_column_statistics

    def _build_index(self, geometry_columns, num_threads=None):
        can_use_statistics = [
            type.coord_type == _ga.CoordType.SEPARATE for type in self.geometry_types
        ]

        normal_stats_cols = list(geometry_columns)

        if self._use_column_statistics and any(can_use_statistics):
            bbox_stats_cols = []
            for col, use_stats in zip(geometry_columns, can_use_statistics):
                if use_stats:
                    bbox_stats_cols.append(col)
                    normal_stats_cols.remove(col)

            bbox_stats = self._build_index_using_stats(bbox_stats_cols)
            normal_stats = super()._build_index(normal_stats_cols, num_threads)

            stats_by_name = {}
            for col, stat in zip(bbox_stats_cols, bbox_stats):
                stats_by_name[col] = stat
            for col in normal_stats_cols:
                stats_by_name[col] = normal_stats.column(col)

            stat_cols = [stats_by_name[col] for col in geometry_columns]
            return _pa.table(
                [normal_stats.column(0)] + stat_cols,
                names=["_fragment_index"] + list(geometry_columns),
            )
        else:
            return super()._build_index(geometry_columns, num_threads)

    def _build_index_using_stats(self, geometry_columns):
        column_types = [self.schema.field(col).type for col in geometry_columns]
        parquet_fields_before = ParquetRowGroupGeoDataset._count_fields_before(
            self.schema
        )
        parquet_fields_before = {k[0]: v for k, v in parquet_fields_before}
        parquet_fields_before = [parquet_fields_before[col] for col in geometry_columns]
        coord_depth = [type.n_offsets for type in column_types]
        parquet_indices = [
            fields_before + depth
            for fields_before, depth in zip(parquet_fields_before, coord_depth)
        ]
        return self._parquet_field_boxes(parquet_indices)

    def _parquet_field_boxes(self, parquet_indices):
        boxes = [[] for item in parquet_indices]
        pq_file = None
        last_row_group = None
        for row_group, fragment in zip(self._row_group_ids, self.get_fragments()):
            if pq_file is None or row_group < last_row_group:
                pq_file = _pq.ParquetFile(
                    fragment.path, filesystem=self._parent.filesystem
                )

            metadata = pq_file.metadata.row_group(row_group)
            for i, parquet_index in enumerate(parquet_indices):
                if parquet_index is None:
                    boxes[i].append(None)
                    continue

                stats_x = metadata.column(parquet_index).statistics
                stats_y = metadata.column(parquet_index + 1).statistics
                # TODO: Check that these exist and return None if stats are missing
                boxes[i].append(
                    {
                        "xmin": stats_x.min,
                        "xmax": stats_x.max,
                        "ymin": stats_y.min,
                        "ymax": stats_y.max,
                    }
                )

            last_row_group = row_group

        type_field_names = ["xmin", "xmax", "ymin", "ymax"]
        type_fields = [_pa.field(name, _pa.float64()) for name in type_field_names]
        type = _pa.struct(type_fields)
        return [_pa.array(box, type=type) for box in boxes]

    @staticmethod
    def _count_fields_before(field, fields_before=None, path=(), count=0):
        """Helper to find the parquet column index of a given field path"""

        if isinstance(field, _pa.Schema):
            fields_before = []
            for i in range(len(field.types)):
                count = ParquetRowGroupGeoDataset._count_fields_before(
                    field.field(i), fields_before, path, count
                )
            return fields_before
        elif _types.is_nested(field.type):
            path = path + (field.name,)
            fields_before.append((path, count))
            for i in range(field.type.num_fields):
                count = ParquetRowGroupGeoDataset._count_fields_before(
                    field.type.field(i), fields_before, path, count
                )
            return count
        else:
            fields_before.append((path + (field.name,), count))
            return count + 1
