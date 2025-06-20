
main() {
  local -r repo_url="https://github.com/apache/arrow-nanoarrow"
  # Check releases page: https://github.com/apache/arrow-nanoarrow/releases/
  local -r commit_sha=b3f35570b92ae11f9133996cf465d94d47993e7c

  echo "Fetching $commit_sha from $repo_url"
  SCRATCH=$(mktemp -d)
  trap 'rm -rf "$SCRATCH"' EXIT

  local -r tarball="$SCRATCH/nanoarrow.tar.gz"
  wget -O "$tarball" "$repo_url/archive/$commit_sha.tar.gz"
  tar --strip-components 1 -C "$SCRATCH" -xf "$tarball"

  # Remove previous bundle
  rm -rf src/geoarrow/nanoarrow

  # Build the bundle
  python "${SCRATCH}/ci/scripts/bundle.py" \
      --include-output-dir=src/geoarrow \
      --source-output-dir=src/geoarrow/nanoarrow
}

main
