
for f in nanoarrow.h nanoarrow.c; do
  curl -L \
    https://raw.githubusercontent.com/apache/arrow-nanoarrow/apache-arrow-nanoarrow-0.2.0/dist/$f \
    -o src/geoarrow/$f
done
