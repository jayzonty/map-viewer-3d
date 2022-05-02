#ifndef CHUNK_DATA_SOURCE_HEADER
#define CHUNK_DATA_SOURCE_HEADER

#include "Map/ChunkData.hpp"

#include <string>

/**
 * Base class for chunk data sources
 */ 
class ChunkDataSource
{
public:
    /**
     * @brief Constructor
     */
    ChunkDataSource()
    {
    }

    /**
     * @brief Destructor
     */
    virtual ~ChunkDataSource()
    {
    }

    /**
     * @brief Retrieves the chunk data
     * @param[in] min Minimum lon/lat point for the bounds
     * @param[in] max Maximum lon/lat point for the bounds
     * @param[out] outChunkData ChunkData object that will contain the retrieved chunk data
     * @return True if the operation was successful.
     */
    virtual bool Retrieve(const glm::vec2 &min, const glm::vec2 &max, ChunkData &outChunkData) = 0;
};

#endif // CHUNK_DATA_SOURCE_HEADER
