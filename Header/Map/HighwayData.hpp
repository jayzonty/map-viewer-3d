#ifndef HIGHWAY_DATA_INCLUDED
#define HIGHWAY_DATA_INCLUDED

#include <glm/glm.hpp>

#include <vector>

/**
 * Based on OpenStreetMap, a highway can be any road, route, transport avenue
 */
struct HighwayData
{
    uint32_t numLanes = 1;              // Number of lanes in the highway
    double roadWidth = 2.0;             // Road width (meters)

    std::vector<glm::dvec2> points;     // List of points in the highway path (lon/lat, world-space)
};

#endif // HIGHWAY_DATA_INCLUDED
