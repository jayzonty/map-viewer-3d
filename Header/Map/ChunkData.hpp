#ifndef CHUNK_DATA_HEADER
#define CHUNK_DATA_HEADER

#include "BuildingData.hpp"
#include "HighwayData.hpp"

#include <glm/fwd.hpp>

/**
 * Struct containing data about a chunk
 */
struct ChunkData
{
    glm::dvec2 position;                    // Center lon/lat in world-space

    std::vector<BuildingData> buildings;    // List of building data
    std::vector<HighwayData> highways;      // List of highway data
};

#endif // CHUNK_DATA_HEADER
