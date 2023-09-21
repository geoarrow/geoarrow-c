
#' Infer a GeoArrow-native type from a vector
#'
#' @param handleable An object implementing [wk_vector_meta()] and [wk_meta()].
#' @param promote_multi Use `TRUE` to return a MULTI type when both normal and
#'   MULTI elements are in the same array.
#' @param coord_type Specify the coordinate type to use if returning
#'
#' @return A [nanoarrow_schema][as_nanoarrow_schema]
#' @export
#'
#' @examples
#' geoarrow_infer_schema_default(wk::wkt("POINT (0 1)"))
#'
geoarrow_infer_schema_default <- function(handleable, promote_multi = TRUE,
                                          coord_type = NULL) {
  if (is.null(coord_type)) {
    coord_type <- enum$CoordType$SEPARATE
  }

  # try vector_meta (doesn't iterate along features)
  vector_meta <- wk::wk_vector_meta(handleable)
  all_types <- vector_meta$geometry_type

  has_mising_info <- is.na(vector_meta$geometry_type) ||
    (vector_meta$geometry_type == 0L) ||
    is.na(vector_meta$has_z) ||
    is.na(vector_meta$has_m)

  if (has_mising_info) {
    # Fall back on calculation from wk_meta(). This would be better with
    # the unique_geometry_types kernel (because it has the option to disregard
    # empties).

    meta <- wk::wk_meta(handleable)
    unique_types <- sort(unique(meta$geometry_type))

    if (length(unique_types) == 1) {
      vector_meta$geometry_type <- unique_types
    } else if (promote_multi && identical(unique_types, c(1L, 4L))) {
      vector_meta$geometry_type <- 4L
    } else if (promote_multi && identical(unique_types, c(2L, 5L))) {
      vector_meta$geometry_type <- 5L
    } else if (promote_multi && identical(unique_types, c(3L, 6L))) {
      vector_meta$geometry_type <- 6L
    }

    vector_meta$has_z <- any(meta$has_z, na.rm = TRUE)
    vector_meta$has_m <- any(meta$has_m, na.rm = TRUE)
  }

  geometry_type <- names(enum$GeometryType)[vector_meta$geometry_type + 1L]
  if (!isTRUE(geometry_type %in% names(enum$GeometryType[2:7]))) {
    return(wk_geoarrow_schema(handleable, na_extension_wkb))
  }

  dims <- if (vector_meta$has_z && vector_meta$has_m) {
    "XYZM"
  } else if (vector_meta$has_z) {
    "XYZ"
  } else if (vector_meta$has_m) {
    "XYM"
  } else {
    "XY"
  }

  wk_geoarrow_schema(
    handleable,
    na_extension_geoarrow,
    geometry_type,
    dimensions = dims,
    coord_type = coord_type
  )
}
