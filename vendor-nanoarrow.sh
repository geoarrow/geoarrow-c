
for f in nanoarrow.h nanoarrow.c; do
  curl -L \
    https://raw.githubusercontent.com/apache/arrow-nanoarrow/3eee79a62dba215b32ef69c63b9b793c5fd6ba78/dist/$f \
    -o src/geoarrow/$f
done

sed -i.bak \
  -e 's|// #define NANOARROW_NAMESPACE YourNamespaceHere|#include "geoarrow_config.h"|' \
  src/geoarrow/nanoarrow.h
rm src/geoarrow/nanoarrow.h.bak
