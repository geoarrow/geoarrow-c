
# If we are building within the repo, copy the C library sources
source_files_src <- list.files(
  "../../src/geoarrow",
  "\\.(c|cc)$",
  recursive = TRUE
)
source_files_src <- source_files_src[!grepl("_test\\.cc$", source_files_src)]

# All source files just go in src/
source_files_dst <- basename(source_files_src)

# ...and the headers
header_files_src <- list.files(
  "../../src/geoarrow", "\\.(h|hpp)$",
  recursive = TRUE
)
header_files_src <- header_files_src[!grepl("testing\\.hpp$", header_files_src)]

# Headers keep the directory structure
header_files_dst <- header_files_src

files_to_vendor <- file.path(
  "../../src/geoarrow",
  c(source_files_src, header_files_src)
)

files_dst <- file.path(
  "src",
  c(source_files_dst, header_files_dst)
)

if (all(file.exists(files_to_vendor))) {
  n_removed <- sum(suppressWarnings(file.remove(files_dst)))
  if (n_removed > 0) {
    cat(sprintf("Removed %d previously vendored files from src/\n", n_removed))
  }

  dst_dirs <- setdiff(unique(dirname(files_dst)), "src")
  unlink(dst_dirs, recursive = TRUE)

  cat(
    sprintf(
      "Vendoring files from geoarrow-c to src/:\n%s\n",
      paste("-", files_to_vendor, collapse = "\n")
    )
  )

  for (dir in dst_dirs) {
    if (!dir.exists(dir)) {
      dir.create(dir, recursive = TRUE)
    }
  }

  if (all(file.copy(files_to_vendor, files_dst))) {
    cat("All files successfully copied to src/\n")
  } else {
    stop("Failed to vendor all files")
  }
}
