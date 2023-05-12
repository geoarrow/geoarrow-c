import json
from concurrent.futures import ThreadPoolExecutor, wait

import pyarrow as pa

from . import _type
from ._array import array
from ._kernel import Kernel

_max_workers = 1

def set_max_workers(max_workers=None):
    global _max_workers
    if max_workers is None:
        max_workers = pa.cpu_count()

    prev = _max_workers
    _max_workers = max_workers
    return prev

def obj_as_array_or_chunked(obj_in):
    if (isinstance(obj_in, pa.Array) or isinstance(obj_in, pa.ChunkedArray)) and isinstance(obj_in.type, _type.VectorType):
        return obj_in
    else:
        return array(obj_in)

def construct_kernel_and_push1(kernel_constructor, obj, args):
    kernel = kernel_constructor(obj.type, **args)
    return kernel.push(obj)

def push_all(kernel_constructor, obj, args=None, is_agg=False, max_workers=None, result=True):
    if args is None:
        args = {}

    if is_agg:
        kernel = kernel_constructor(**args)
        kernel.push(obj)
        return kernel.finish()

    if max_workers is None:
        max_workers = _max_workers

    if isinstance(obj, pa.Array) or obj.num_chunks <= 1 or max_workers == 1:
        return construct_kernel_and_push1(kernel_constructor, obj, args)

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = []
        for chunk in obj.chunks:
            future = executor.submit(construct_kernel_and_push1, kernel_constructor, chunk, args)
            futures.append(future)
        wait(futures, return_when="FIRST_EXCEPTION")

        chunks_out = [future.result() for future in futures]
        if result:
            return pa.chunked_array(chunks_out)


def parse_all(obj):
    obj = obj_as_array_or_chunked(obj)

    # Non-wkb or wkt types are a no-op here since they don't need parsing
    if isinstance(obj.type, _type.WkbType) or isinstance(obj.type, _type.WktType):
        push_all(Kernel.visit_void_agg, obj, result=False)

    return None

def as_wkt(obj):
    obj = obj_as_array_or_chunked(obj)

    if isinstance(obj.type, _type.WktType):
        return obj

    return push_all(obj)


