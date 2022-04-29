#ifndef HIGHWAY_DATA_INCLUDED
#define HIGHWAY_DATA_INCLUDED

#include <glm/glm.hpp>

#include <vector>

/**
 * Based on OpenStreetMap, a highway can be any road, route, transport avenue
 */
struct HighwayData
{
    uint32_t numLanes = 1;
    double roadWidth = 2.0;
    glm::dvec2 position;
    std::vector<glm::dvec2> points;
};

#endif // HIGHWAY_DATA_INCLUDED
