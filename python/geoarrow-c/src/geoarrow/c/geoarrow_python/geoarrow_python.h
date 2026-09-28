
#ifndef GEOARROW_PYTHON_H_INCLUDED
#define GEOARROW_PYTHON_H_INCLUDED

#include <Python.h>
#include <stdint.h>
#include "geoarrow/geoarrow.h"

static void PyGeoArrowBufferFree(uint8_t* ptr, int64_t size, void* private_data) {
  // Arrow release callbacks may run on a foreign or detached thread. An attached
  // thread state is required for Python C API calls, including on free-threaded
  // builds where PyGILState_Ensure() does not necessarily acquire a GIL.
  PyGILState_STATE python_state = PyGILState_Ensure();
  PyObject* obj = (PyObject*)private_data;
  Py_DECREF(obj);
  PyGILState_Release(python_state);
}

static GeoArrowErrorCode GeoArrowBuilderSetPyBuffer(struct GeoArrowBuilder* builder,
                                                    int64_t i, PyObject* obj,
                                                    const void* ptr, int64_t size) {
  GeoArrowBufferView view;
  view.data = (const uint8_t*)ptr;
  view.size_bytes = size;

  // Take ownership before making the release callback reachable. This function is
  // currently called from Cython with a thread state attached, but ensure one here
  // so that all Python reference count operations in this header are self-contained.
  PyGILState_STATE python_state = PyGILState_Ensure();
  Py_INCREF(obj);

  // This only fails with ENOMEM for a small allocation before the buffer is added
  // to the builder.
  int result =
      GeoArrowBuilderSetOwnedBuffer(builder, i, view, &PyGeoArrowBufferFree, obj);
  if (result != GEOARROW_OK) {
    Py_DECREF(obj);
    PyGILState_Release(python_state);
    return result;
  }

  PyGILState_Release(python_state);
  return GEOARROW_OK;
}

#endif
