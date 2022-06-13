#ifndef TILE_DATA_HEADER
#define TILE_DATA_HEADER

#include "BuildingData.hpp"
#include "Core/Rect.hpp"
#include "HighwayData.hpp"

#include <glm/fwd.hpp>

/**
 * Struct containing data about a tile
 */
struct TileData
{
    glm::dvec2 center;                      // Tile center (lon/lat, world-space)
    RectD bounds;                           // Tile bounds (lon/lat, world-space)

    std::vector<BuildingData> buildings;    // List of building data
    std::vector<HighwayData> highways;      // List of highway data
};

#endif // TILE_DATA_HEADER
