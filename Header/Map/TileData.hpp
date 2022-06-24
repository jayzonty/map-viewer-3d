#ifndef TILE_DATA_HEADER
#define TILE_DATA_HEADER

#include "BuildingData.hpp"
#include "Core/Rect.hpp"
#include "HighwayData.hpp"

#include <glm/fwd.hpp>

// Struct containing data about a water feature
struct WaterFeatureData
{
    std::vector<glm::dvec2> outline;        // Outline of the water feature geometry
};

/**
 * Struct containing data about a tile
 */
struct TileData
{
    glm::ivec2 index;                       // Tile index
    RectD bounds;                           // Tile bounds (lon/lat, world-space)

    std::vector<BuildingData> buildings;    // List of building data
    std::vector<HighwayData> highways;      // List of highway data
    std::vector<WaterFeatureData> waterFeatures;    // List of water features
};

#endif // TILE_DATA_HEADER
