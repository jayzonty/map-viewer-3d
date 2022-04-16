#ifndef MAP_DATA_HEADER
#define MAP_DATA_HEADER

#include "BuildingData.hpp"
#include "glm/fwd.hpp"
#include "tinyxml2.h"

#include <string>
#include <vector>

class MapData
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

    const double SCALE = 0.05;
    const double METERS_PER_LEVEL = 3.0;

private:
    glm::vec2 m_position;
    std::vector<BuildingData> m_buildings;

public:
    MapData();
    ~MapData();

    bool Parse(const std::string &mapFilePath);

    const std::vector<BuildingData>* GetBuildings()
    {
        return &m_buildings;
    }

private:
    bool HasChildTag(tinyxml2::XMLElement *parent, const char *key);
    const tinyxml2::XMLAttribute* GetChildTagValue(tinyxml2::XMLElement *parent, const char *key);
};

#endif // MAP_DATA_HEADER
