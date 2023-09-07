
# Use GEOS' version, which includes some UBSAN fixes and removal of trailing zeroes
curl -L https://github.com/libgeos/geos/archive/refs/tags/3.12.0.tar.gz |
    tar -xzf -

rm -rf src/geoarrow/ryu

cp -R geos-3.12.0/src/deps/ryu src/geoarrow

# Use our own symbol prefix
sed -i.bak "s/geos_/GeoArrow/" src/geoarrow/ryu/d2s.c
sed -i.bak "s/geos_/GeoArrow/" src/geoarrow/ryu/ryu.h

# Make it easier for tests to pass with and without ryu
sed -i.bak "s/NaN/nan/" src/geoarrow/ryu/common.h
rm src/geoarrow/ryu/*.bak

rm -rf geos-3.12.0
