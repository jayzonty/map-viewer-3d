#ifndef OSM_CHUNK_DATA_SOURCE_HEADER
#define OSM_CHUNK_DATA_SOURCE_HEADER

#include "Map/ChunkDataSource.hpp"
#include "Map/ChunkData.hpp"
#include "tinyxml2.h"

#include <glm/fwd.hpp>

/**
 * Source of chunk data from OSM
 */
class OSMChunkDataSource : public ChunkDataSource
{
private:
    const char *OSM_ELEMENT_STR = "osm";
    const char *NODE_ELEMENT_STR = "node";
    const char *NODE_ID_ATTRIBUTE_STR = "id";
    const char *NODE_LAT_ATTRIBUTE_STR = "lat";
    const char *NODE_LON_ATTRIBUTE_STR = "lon";
    const char *WAY_ELEMENT_STR = "way";
    const char *WAY_NODE_ELEMENT_STR = "nd";
    const char *WAY_NODE_REF_ATTRIBUTE_STR = "ref";
    const char *TAG_ELEMENT_STR = "tag";
    const char *TAG_KEY_ATTRIBUTE_STR = "k";
    const char *TAG_VALUE_ATTRIBUTE_STR = "v";

    const char *BUILDING_TAG_KEY_STR = "building";
    const char *BUILDING_PART_TAG_KEY_STR = "building:part";
    const char *BUILDING_LEVELS_TAG_KEY_STR = "building:levels";
    const char *BUILDING_MIN_LEVELS_TAG_KEY_STR = "building:min_levels";
    const char *BUILDING_HEIGHT_TAG_KEY_STR = "height";
    const char *BUILDING_MIN_HEIGHT_TAG_KEY_STR = "min_height";

    const char *HIGHWAY_TAG_KEY_STR = "highway";
    const char *HIGHWAY_LANES_TAG_KEY_STR = "lanes";

    const double SCALE = 0.05;
    const double METERS_PER_LEVEL = 3.0;
    const double PRIMARY_HIGHWAY_LANE_WIDTH_METERS = 2.0; 
    const double RESIDENTIAL_HIGHWAY_LANE_WIDTH_METERS = 1.0; 

public:
    /**
     * @brief Constructor
     */
    OSMChunkDataSource();

    /**
     * @brief Destructor
     */
    ~OSMChunkDataSource();

    /**
     * @brief Retrieves the chunk data
     * @param[in] min Minimum lon/lat point for the bounds
     * @param[in] max Maximum lon/lat point for the bounds
     * @param[out] outChunkData ChunkData object that will contain the retrieved chunk data
     * @return True if the operation was successful.
     */
    bool Retrieve(const glm::vec2 &min, const glm::vec2 &max, ChunkData &outChunkData) override;

private:
    /**
     * @brief Retrieves chunk data from the given xml document
     * @param[in] xml XML document object
     * @param[out] outChunkData ChunkData object that will contain the retrieved chunk data
     * @return True if the operation was successful.
     */
    bool RetrieveFromXML(const tinyxml2::XMLDocument &xml, ChunkData &outChunkData);

    bool HasChildTag(const tinyxml2::XMLElement *parent, const char *key);
    const tinyxml2::XMLAttribute* GetChildTagValue(const tinyxml2::XMLElement *parent, const char *key);
};

#endif // OSM_CHUNK_DATA_SOURCE_HEADER