
#ifndef GEOARROW_PYTHON_H_INCLUDED
#define GEOARROW_PYTHON_H_INCLUDED

#include <Python.h>
#include <stdint.h>
#include "geoarrow/geoarrow.h"

static void PyGeoArrowBufferFree(uint8_t* ptr, int64_t size, void* private_data) {
  // Acquire the GIL? This buffer very well maybe freed from another thread.
  PyObject* obj = (PyObject*)private_data;
  Py_DECREF(obj);
}

static GeoArrowErrorCode GeoArrowBuilderSetPyBuffer(struct GeoArrowBuilder* builder,
                                                    int64_t i, PyObject* obj,
                                                    const void* ptr, int64_t size) {
  // Acquire the GIL? Or maybe not since this should never be initialized from antying
  // that isn't a cython <PyObject*> cast.
  GeoArrowBufferView view;
  view.data = (const uint8_t*)ptr;
  view.size_bytes = size;

  // This only fails with ENOMEM for a small allocation before the buffer is added
  // to the builder.
  int result =
      GeoArrowBuilderSetOwnedBuffer(builder, i, view, &PyGeoArrowBufferFree, obj);
  if (result != GEOARROW_OK) {
    return result;
  }

  Py_INCREF(obj);
  return GEOARROW_OK;
}

#endif
