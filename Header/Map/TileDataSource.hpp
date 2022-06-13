#ifndef TILE_DATA_SOURCE_HEADER
#define TILE_DATA_SOURCE_HEADER

#include "Map/TileData.hpp"

#include <string>

/**
 * Base class for tile data sources
 */ 
class TileDataSource
{
public:
    /**
     * @brief Constructor
     */
    TileDataSource()
    {
    }

    /**
     * @brief Destructor
     */
    virtual ~TileDataSource()
    {
    }

    /**
     * @brief Retrieves the tile data
     * @param[in] tileIndex Tile index
     * @param[in] zoomLevel Zoom level
     * @param[out] outTileData TileData object that will contain the retrieved tile data
     * @return True if the operation was successful.
     */
    virtual bool Retrieve(const glm::ivec2 &tileIndex, const int &zoomLevel, TileData &outTileData) = 0;
};

#endif // TILE_DATA_SOURCE_HEADER
