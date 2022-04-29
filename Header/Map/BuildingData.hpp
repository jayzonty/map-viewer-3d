#ifndef BUILDING_DATA_HEADER
#define BUILDING_DATA_HEADER

#include <glm/glm.hpp>

#include <vector>

struct BuildingData
{
    glm::dvec2 position;
    double heightInMeters = 6.0;
    double heightFromGround = 0.0;
    std::vector<glm::dvec2> outline;
};

#endif // BUILDING_DATA_HEADER
