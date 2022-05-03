#ifndef BUILDING_DATA_HEADER
#define BUILDING_DATA_HEADER

#include <glm/glm.hpp>

#include <vector>

struct BuildingData
{
    double heightInMeters = 6.0;        // Height of the building (meters)
    double heightFromGround = 0.0;      // Height of the building base from the ground (meters)

    std::vector<glm::dvec2> outline;    // List of points in the building outline (xy, world-space)
};

#endif // BUILDING_DATA_HEADER
