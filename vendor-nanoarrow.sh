


main() {
  local -r repo_url="https://github.com/paleolimbot/arrow-nanoarrow"
  # Check releases page: https://github.com/apache/arrow-nanoarrow/releases/
  local -r commit_sha=ec7a2a1a43922ef4c5b025a22194d8f121507fb9

  echo "Fetching $commit_sha from $repo_url"
  SCRATCH=$(mktemp -d)
  trap 'rm -rf "$SCRATCH"' EXIT

  local -r tarball="$SCRATCH/nanoarrow.tar.gz"
  wget -O "$tarball" "$repo_url/archive/$commit_sha.tar.gz"
  tar --strip-components 1 -C "$SCRATCH" -xf "$tarball"

  # Build the bundle
  python "${SCRATCH}/ci/scripts/bundle.py" \
      --include-output-dir=src/geoarrow \
      --source-output-dir=src/geoarrow \
      --header-namespace=

  rm src/geoarrow/nanoarrow.hpp

  sed -i.bak \
    -e 's|// #define NANOARROW_NAMESPACE YourNamespaceHere|// When testing we use nanoarrow.h, but geoarrow_config.h will not exist in bundled\
// mode. In the tests we just have to make sure geoarrow.h is always included first.\
#if !defined(GEOARROW_CONFIG_H_INCLUDED)\
#include "geoarrow_config.h"\
#endif|' \
    src/geoarrow/nanoarrow.h
  rm src/geoarrow/nanoarrow.h.bak

  clang-format -i src/geoarrow/nanoarrow.h
  clang-format -i src/geoarrow/nanoarrow.c
}

main
