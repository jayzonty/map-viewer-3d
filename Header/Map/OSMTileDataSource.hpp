#ifndef OSM_TILE_DATA_SOURCE_HEADER
#define OSM_TILE_DATA_SOURCE_HEADER

#include "Map/BuildingData.hpp"
#include "Map/TileDataSource.hpp"
#include "Map/TileData.hpp"

#include <glm/fwd.hpp>
#include <tinyxml2.h>

#include <map>

/**
 * Source of tile data from OSM
 */
class OSMTileDataSource : public TileDataSource
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

    const double METERS_PER_LEVEL = 3.0;
    const double PRIMARY_HIGHWAY_LANE_WIDTH_METERS = 2.0; 
    const double RESIDENTIAL_HIGHWAY_LANE_WIDTH_METERS = 1.0; 

public:
    /**
     * @brief Constructor
     */
    OSMTileDataSource();

    /**
     * @brief Destructor
     */
    ~OSMTileDataSource();

    /**
     * @brief Retrieves the tile data
     * @param[in] tileIndex Tile index
     * @param[in] zoomLevel Zoom level
     * @param[out] outTileData TileData object that will contain the retrieved tile data
     * @return True if the operation was successful.
     */
    bool Retrieve(const glm::ivec2 &tileIndex, const int &zoomLevel, TileData &outTileData) override;

private:
    /**
     * @brief Retrieves tile data from the given xml document
     * @param[in] xml XML document object
     * @param[out] outTileData TileData object that will contain the retrieved tile data
     * @return True if the operation was successful.
     */
    bool RetrieveFromXML(const tinyxml2::XMLDocument &xml, TileData &outTileData);

    /**
     * @brief Retrieves building data from the given xml element
     * @param[in] element XML element object
     * @param[in] nodeIDToLonLat Map containing the mapping between a node ID and its lon/lat position
     * @param[out] outBuildingData BuildingData object that will contain the retrieved building data
     * @return True if the operation was successful.
     */
    bool RetrieveBuildingData(const tinyxml2::XMLElement *element, const std::map<int32_t, glm::dvec2> &nodeIDToLonLat, BuildingData &outBuildingData);

    /**
     * @brief Retrieves highway data from the given xml element
     * @param[in] element XML element object
     * @param[in] nodeIDToLonLat Map containing the mapping between a node ID and its lon/lat position
     * @param[out] outHighwayData HighwayData object that will contain the retrieved highway data
     * @return True if the operation was successful.
     */
    bool RetrieveHighwayData(const tinyxml2::XMLElement *element, const std::map<int32_t, glm::dvec2> &nodeIDToLonLat, HighwayData &outHighwayData);

    bool HasChildTag(const tinyxml2::XMLElement *parent, const char *key);
    const tinyxml2::XMLAttribute* GetChildTagValue(const tinyxml2::XMLElement *parent, const char *key);

    /**
     * @brief Gets the bounding box (in lon-lat) of the tile identified by the provided tile index
     * @param[in] tileX Tile index in the x-axis
     * @param[in] tileY Tile index in the y-axis
     * @param[in] zoomLevel Zoom level
     * @return Bounding box (in lon-lat) of the tile
     */
    RectD GetLonLatBoundsFromTile(const int &tileX, const int &tileY, const int &zoomLevel);
};

#endif // OSM_TILE_DATA_SOURCE_HEADER
