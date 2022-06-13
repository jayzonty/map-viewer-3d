#include "Map/OSMTileDataSource.hpp"

#include "Map/TileDataSource.hpp"
#include "Util/GeometryUtils.hpp"

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <tinyxml2.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

/**
 * @brief Constructor
 */
OSMTileDataSource::OSMTileDataSource()
    : TileDataSource()
{
}

/**
 * @brief Destructor
 */
OSMTileDataSource::~OSMTileDataSource()
{
}

/**
 * @brief Retrieves the tile data
 * @param[in] tileIndex Tile index
 * @param[in] zoomLevel Zoom level
 * @param[out] outTileData TileData object that will contain the retrieved tile data
 * @return True if the operation was successful.
 */
bool OSMTileDataSource::Retrieve(const glm::ivec2 &tileIndex, const int &zoomLevel, TileData &outTileData)
{
    std::stringstream ss;
    ss << "Resources/map_" << zoomLevel << "-" << tileIndex.x << "-" << tileIndex.y << ".osm";
    std::string fileName = ss.str();

    tinyxml2::XMLDocument document;
    if (document.LoadFile(fileName.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cerr << "[OSMTileDataSource] Cannot retrieve map " << fileName << std::endl;
        return false;
    }

    outTileData.index = tileIndex;
    return RetrieveFromXML(document, outTileData);
}

/**
 * @brief Retrieves tile data from the given xml document
 * @param[in] xml XML document object
 * @param[out] outTileData TileData object that will contain the retrieved tile data
 * @return True if the operation was successful.
 */
bool OSMTileDataSource::RetrieveFromXML(const tinyxml2::XMLDocument &xml, TileData &outTileData)
{
    const tinyxml2::XMLElement *rootElement = xml.FirstChildElement(OSM_ELEMENT_STR);

    const tinyxml2::XMLElement *boundsElement = rootElement->FirstChildElement("bounds");
    outTileData.bounds.min.x = boundsElement->DoubleAttribute("minlon");
    outTileData.bounds.min.y = boundsElement->DoubleAttribute("minlat");
    outTileData.bounds.max.x = boundsElement->DoubleAttribute("maxlon");
    outTileData.bounds.max.y = boundsElement->DoubleAttribute("maxlat");

    std::map<int32_t, glm::dvec2> nodeIDToLonLat;

    const tinyxml2::XMLElement *nodeElement = xml.FirstChildElement(OSM_ELEMENT_STR)->FirstChildElement(NODE_ELEMENT_STR);
    while (nodeElement != nullptr)
    {
        int32_t nodeId = nodeElement->IntAttribute(NODE_ID_ATTRIBUTE_STR);
        if (nodeId != 0) // TODO: Not sure if 0 is a valid node ID
        {
            double lon = nodeElement->FloatAttribute(NODE_LON_ATTRIBUTE_STR);
            double lat = nodeElement->FloatAttribute(NODE_LAT_ATTRIBUTE_STR);
            nodeIDToLonLat[nodeId] = glm::dvec2(lon, lat);
        }
        nodeElement = nodeElement->NextSiblingElement(NODE_ELEMENT_STR);
    }

    const tinyxml2::XMLElement *wayElement = xml.FirstChildElement(OSM_ELEMENT_STR)->FirstChildElement(WAY_ELEMENT_STR);
    while (wayElement != nullptr)
    {
        if (HasChildTag(wayElement, BUILDING_TAG_KEY_STR)
            || HasChildTag(wayElement, BUILDING_PART_TAG_KEY_STR))
        {
            outTileData.buildings.emplace_back();
            if (!RetrieveBuildingData(wayElement, nodeIDToLonLat, outTileData.buildings.back()))
            {
                outTileData.buildings.pop_back();
            }
        }
        else if (HasChildTag(wayElement, HIGHWAY_TAG_KEY_STR))
        {
            outTileData.highways.emplace_back();
            if (!RetrieveHighwayData(wayElement, nodeIDToLonLat, outTileData.highways.back()))
            {
                outTileData.highways.pop_back();
            }
        }

        wayElement = wayElement->NextSiblingElement(WAY_ELEMENT_STR);
    }

    return true;
}

/**
 * @brief Retrieves building data from the given xml element
 * @param[in] element XML element object
 * @param[in] nodeIDToLonLat Map containing the mapping between a node ID and its lon/lat position
 * @param[out] outBuildingData BuildingData object that will contain the retrieved building data
 * @return True if the operation was successful.
 */
bool OSMTileDataSource::RetrieveBuildingData(const tinyxml2::XMLElement *element, const std::map<int32_t, glm::dvec2> &nodeIDToLonLat, BuildingData &outBuildingData)
{
    const tinyxml2::XMLElement *nodeRefElement = element->FirstChildElement(WAY_NODE_ELEMENT_STR);
    while (nodeRefElement != nullptr)
    {
        int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
        if (nodeIDToLonLat.find(nodeId) != nodeIDToLonLat.end())
        {
            double lon = nodeIDToLonLat.at(nodeId).x;
            double lat = nodeIDToLonLat.at(nodeId).y;
            outBuildingData.outline.emplace_back(lon, lat);
        }
        nodeRefElement = nodeRefElement->NextSiblingElement(WAY_NODE_ELEMENT_STR);
    }

    if (outBuildingData.outline.size() == 0)
    {
        return false;
    }

    if (outBuildingData.outline[0] == outBuildingData.outline.back())
    {
        outBuildingData.outline.pop_back();
    }

    const tinyxml2::XMLAttribute *heightAttrib = GetChildTagValue(element, BUILDING_HEIGHT_TAG_KEY_STR);
    const tinyxml2::XMLAttribute *minHeightAttrib = GetChildTagValue(element, BUILDING_MIN_HEIGHT_TAG_KEY_STR);
    const tinyxml2::XMLAttribute *buildingLevelsAttrib = GetChildTagValue(element, BUILDING_LEVELS_TAG_KEY_STR);
    const tinyxml2::XMLAttribute *buildingMinLevelsAttrib = GetChildTagValue(element, BUILDING_MIN_LEVELS_TAG_KEY_STR);
    // height has priority over building:levels
    if (heightAttrib != nullptr)
    {
        outBuildingData.heightInMeters = heightAttrib->DoubleValue();
    }
    else if (buildingLevelsAttrib != nullptr)
    {
        outBuildingData.heightInMeters = buildingLevelsAttrib->DoubleValue() * METERS_PER_LEVEL;
    }
    // min_height has priority over building:min_levels
    if (minHeightAttrib != nullptr)
    {
        outBuildingData.heightFromGround = minHeightAttrib->DoubleValue();
        if (heightAttrib != nullptr)
        {
            outBuildingData.heightInMeters -= minHeightAttrib->DoubleValue();
        }
    }
    else if (buildingMinLevelsAttrib != nullptr)
    {
        outBuildingData.heightFromGround = buildingMinLevelsAttrib->DoubleValue() * METERS_PER_LEVEL;
        if (heightAttrib != nullptr)
        {
            outBuildingData.heightInMeters = glm::max(heightAttrib->DoubleValue() - outBuildingData.heightFromGround, METERS_PER_LEVEL);
        }
        else if (buildingLevelsAttrib != nullptr)
        {
            outBuildingData.heightInMeters -= outBuildingData.heightFromGround;
        }
    }

    return true;
}

/**
 * @brief Retrieves highway data from the given xml element
 * @param[in] element XML element object
 * @param[in] nodeIDToLonLat Map containing the mapping between a node ID and its lon/lat position
 * @param[out] outHighwayData HighwayData object that will contain the retrieved highway data
 * @return True if the operation was successful.
 */
bool OSMTileDataSource::RetrieveHighwayData(const tinyxml2::XMLElement *element, const std::map<int32_t, glm::dvec2> &nodeIDToLonLat, HighwayData &outHighwayData)
{
    const tinyxml2::XMLElement *nodeRefElement = element->FirstChildElement(WAY_NODE_ELEMENT_STR);
    while (nodeRefElement != nullptr)
    {
        int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
        if (nodeIDToLonLat.find(nodeId) != nodeIDToLonLat.end())
        {
            double lon = nodeIDToLonLat.at(nodeId).x;
            double lat = nodeIDToLonLat.at(nodeId).y;
            outHighwayData.points.emplace_back(lon, lat);
        }
        nodeRefElement = nodeRefElement->NextSiblingElement(WAY_NODE_ELEMENT_STR);
    }

    if (outHighwayData.points.size() == 0)
    {
        return false;
    }

    double numLanes = 1.0;
    double width = RESIDENTIAL_HIGHWAY_LANE_WIDTH_METERS;
    const tinyxml2::XMLAttribute *highwayTypeAttrib = GetChildTagValue(element, HIGHWAY_TAG_KEY_STR);
    if (highwayTypeAttrib != nullptr)
    {
        if (strcmp(highwayTypeAttrib->Value(), "primary") == 0)
        {
            width = PRIMARY_HIGHWAY_LANE_WIDTH_METERS;
        }
    }
    const tinyxml2::XMLAttribute *lanesAttrib = GetChildTagValue(element, HIGHWAY_LANES_TAG_KEY_STR);
    if (lanesAttrib != nullptr)
    {
        numLanes = lanesAttrib->DoubleValue();
    }
    outHighwayData.roadWidth = width * numLanes;

    return true;
}

bool OSMTileDataSource::HasChildTag(const tinyxml2::XMLElement *parent, const char *key)
{
    const tinyxml2::XMLElement *curr = parent->FirstChildElement(TAG_ELEMENT_STR);
    while (curr != nullptr)
    {
        const tinyxml2::XMLAttribute *attrib = curr->FindAttribute(TAG_KEY_ATTRIBUTE_STR);
        if (attrib != nullptr)
        {
            if (strcmp(attrib->Value(), key) == 0)
            {
                return true;
            }
        }
        curr = curr->NextSiblingElement(TAG_ELEMENT_STR);
    }

    return false;
}

const tinyxml2::XMLAttribute* OSMTileDataSource::GetChildTagValue(const tinyxml2::XMLElement *parent, const char *key)
{
    const tinyxml2::XMLElement *curr = parent->FirstChildElement(TAG_ELEMENT_STR);
    while (curr != nullptr)
    {
        const tinyxml2::XMLAttribute *attrib = curr->FindAttribute(TAG_KEY_ATTRIBUTE_STR);
        if (attrib != nullptr)
        {
            if (strcmp(attrib->Value(), key) == 0)
            {
                return curr->FindAttribute(TAG_VALUE_ATTRIBUTE_STR);
            }
        }
        curr = curr->NextSiblingElement(TAG_ELEMENT_STR);
    }

    return nullptr;
}

/**
 * @brief Gets the bounding box (in lon-lat) of the tile identified by the provided tile index
 * @param[in] tileX Tile index in the x-axis
 * @param[in] tileY Tile index in the y-axis
 * @param[in] zoomLevel Zoom level
 * @return Bounding box (in lon-lat) of the tile
 */
RectD OSMTileDataSource::GetLonLatBoundsFromTile(const int &tileX, const int &tileY, const int &zoomLevel)
{
	int numTilesPerAxis = 1 << zoomLevel;

	glm::dvec2 min = GeometryUtils::TileIndexToLonLat(tileX, tileY, zoomLevel);

	glm::dvec2 max = GeometryUtils::TileIndexToLonLat(tileX + 1, tileY + 1, zoomLevel);
	if (tileX + 1 >= numTilesPerAxis)
	{
		max.x = 180.0f;
	}
	if (tileY + 1 >= numTilesPerAxis)
	{
		max.y = -90.0f;
	}

	double temp = min.y;
	min.y = max.y;
	max.y = temp;

	RectD ret = {};
	ret.min = min;
	ret.max = max;
	return ret;
}
