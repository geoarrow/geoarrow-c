
#include <cstdio>
#include <cstring>
#include <iostream>

#include "geoarrow/geoarrow.hpp"

int main(int argc, char* argv[]) {
  // Use something from the geoarrow c runtime
  std::printf("Version reported by the geoarrow-c runtime '%s'\n", GeoArrowVersion());

  return EXIT_SUCCESS;
}
