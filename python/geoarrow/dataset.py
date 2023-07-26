from concurrent.futures import ThreadPoolExecutor, wait

import pyarrow as _pa
import pyarrow.dataset as _ds
import pyarrow.compute as _compute
import pyarrow.parquet as _pq
from . import pyarrow as _ga
from .pyarrow._kernel import Kernel


class GeoDataset:
    def __init__(self, parent, *args, geometry_columns=None, **kwargs) -> None:
        self._parent = None
        self._index = None
        self._geometry_columns = geometry_columns
        self._fragments = None

        if isinstance(parent, _ds.Dataset):
            self._parent = parent
        else:
            self._parent = _ds.dataset(parent, *args, **kwargs)

    def use_row_groups(self):
        if isinstance(self._parent, _ds.FileSystemDataset) and isinstance(
            self._parent.format, _ds.ParquetFileFormat
        ):
            return ParquetRowGroupGeoDataset(self._parent)
        else:
            raise TypeError("use_row_groups() is only suppoted for Parquet datasets")

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

    def index_fragments(self, num_threads=None):
        if self._index is None:
            self._index = GeoDataset._index_fragments(
                self.get_fragments(), self.geometry_columns, num_threads=num_threads
            )

        return self._index

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

        with ThreadPoolExecutor(max_workers=num_threads) as executor:
            futures = []
            for column in columns:
                for fragment in fragments:
                    future = executor.submit(
                        GeoDataset._index_fragment, fragment, column
                    )
                    futures.append(future)

            wait(futures)

            num_fragments = len(futures) // len(columns)
            results = []
            for i, column in enumerate(columns):
                results.append(
                    [
                        futures[i * num_fragments + j].result()
                        for j in range(num_fragments)
                    ]
                )

            metadata = [_pa.array(range(num_fragments))]
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
    def __init__(self, parent) -> None:
        row_group_fragments = []
        row_group_ids = []

        for file_fragment in parent.get_fragments():
            for i, row_group_fragment in enumerate(file_fragment.split_by_row_group()):
                row_group_fragments.append(row_group_fragment)
                # Keep track of the row group IDs so we can possibly accellerate
                # building an index later where column statistics are supported
                row_group_ids.append(i)

        super().__init__(
            _ds.FileSystemDataset(row_group_fragments, parent.schema, parent.format)
        )
        self._fragments = row_group_fragments
        self._row_group_ids = row_group_ids
