#ifndef BUILDING_DATA_HEADER
#define BUILDING_DATA_HEADER

#include <glm/glm.hpp>

#include <vector>

struct BuildingData
{
    glm::vec2 position;
    double heightInMeters = 6.0;
    double heightFromGround = 0.0;
    std::vector<glm::vec2> outline;
};

#endif // BUILDING_DATA_HEADER
