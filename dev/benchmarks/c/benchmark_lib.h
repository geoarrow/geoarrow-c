
#include <array>

#include "geoarrow.h"

std::array<double, 4> CalculateBoundsGeneric(struct GeoArrowCoordView* coords,
                                             int64_t n_coords);

std::array<double, 4> CalculateBoundsOptimized(struct GeoArrowCoordView* coords,
                                               int64_t n_coords,
                                               enum GeoArrowCoordType coord_type);

std::array<double, 4> CalculateBoundsLoopThenIf(struct GeoArrowCoordView* coords,
                                                int64_t n_coords,
                                                enum GeoArrowCoordType coord_type);
