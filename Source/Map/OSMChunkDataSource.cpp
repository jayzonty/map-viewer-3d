#include "Map/OSMChunkDataSource.hpp"

#include "Map/ChunkDataSource.hpp"
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
OSMChunkDataSource::OSMChunkDataSource()
    : ChunkDataSource()
{
}

/**
 * @brief Destructor
 */
OSMChunkDataSource::~OSMChunkDataSource()
{
}

/**
 * @brief Retrieves the chunk data
 * @param[in] min Minimum lon/lat point for the bounds
 * @param[in] max Maximum lon/lat point for the bounds
 * @param[out] outChunkData ChunkData object that will contain the retrieved chunk data
 * @return True if the operation was successful.
 */
bool OSMChunkDataSource::Retrieve(const glm::vec2 &min, const glm::vec2 &max, ChunkData &outChunkData)
{
    // TODO: The following is just a temporary way of retrieving for now
    const uint32_t ZOOM_LEVEL = 14;
    const uint32_t MAX_PRECISION = 5;

    std::stringstream ss;
    ss << "Resources/map_" << std::fixed << std::setprecision(MAX_PRECISION) << ZOOM_LEVEL << "-" << min.x << "-" << min.y << "-" << max.x << "-" << max.y << ".osm";
    std::string fileName = ss.str();

    tinyxml2::XMLDocument document;
    if (document.LoadFile(fileName.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cerr << "[OSMChunkDataSource] Cannot retrieve map " << fileName << std::endl;
        return false;
    }

    return RetrieveFromXML(document, outChunkData);
}

/**
 * @brief Retrieves chunk data from the given xml document
 * @param[in] xml XML document object
 * @param[out] outChunkData ChunkData object that will contain the retrieved chunk data
 * @return True if the operation was successful.
 */
bool OSMChunkDataSource::RetrieveFromXML(const tinyxml2::XMLDocument &xml, ChunkData &outChunkData)
{
    const tinyxml2::XMLElement *rootElement = xml.FirstChildElement(OSM_ELEMENT_STR);

    glm::dvec2 boundsMin, boundsMax;
    const tinyxml2::XMLElement *boundsElement = rootElement->FirstChildElement("bounds");
    boundsMin.x = boundsElement->DoubleAttribute("minlon");
    boundsMin.y = boundsElement->DoubleAttribute("minlat");
    boundsMax.x = boundsElement->DoubleAttribute("maxlon");
    boundsMax.y = boundsElement->DoubleAttribute("maxlat");
    outChunkData.position = GeometryUtils::LonLatToXY((boundsMin + boundsMax) / 2.0);

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
            outChunkData.buildings.emplace_back();
            if (!RetrieveBuildingData(wayElement, nodeIDToLonLat, outChunkData.buildings.back()))
            {
                outChunkData.buildings.pop_back();
            }
        }
        else if (HasChildTag(wayElement, HIGHWAY_TAG_KEY_STR))
        {
            outChunkData.highways.emplace_back();
            if (!RetrieveHighwayData(wayElement, nodeIDToLonLat, outChunkData.highways.back()))
            {
                outChunkData.highways.pop_back();
            }
        }

        wayElement = wayElement->NextSiblingElement(WAY_ELEMENT_STR);
    }

    if (outChunkData.buildings.size() > 0)
    {
        for (size_t i = 0; i < outChunkData.buildings.size(); i++)
        {
            outChunkData.buildings[i].positionInChunk -= outChunkData.position;
            for (size_t j = 0; j < outChunkData.buildings[i].outline.size(); j++)
            {
                glm::dvec2 &a = outChunkData.buildings[i].outline[j];
                glm::dvec2 &b = outChunkData.buildings[i].outline[(j + 1) % outChunkData.buildings[i].outline.size()];
                glm::dvec2 &c = outChunkData.buildings[i].outline[(j + 2) % outChunkData.buildings[i].outline.size()];

                if (GeometryUtils::IsCollinear(a, b, c))
                {
                    outChunkData.buildings[i].outline.erase(outChunkData.buildings[i].outline.begin() + ((j + 1) % outChunkData.buildings[i].outline.size()));
                    --j;
                }
            }

            if (!GeometryUtils::IsPolygonCCW(outChunkData.buildings[i].outline))
            {
                std::reverse(outChunkData.buildings[i].outline.begin(), outChunkData.buildings[i].outline.end());
            }
            // Double check if polygon is now indeed CCW
            if (!GeometryUtils::IsPolygonCCW(outChunkData.buildings[i].outline))
            {
                std::cout << "Polygon is still not CCW!" << std::endl;
            }
        }
    }

    for (size_t i = 0; i < outChunkData.highways.size(); i++)
    {
        outChunkData.highways[i].position -= outChunkData.position;
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
bool OSMChunkDataSource::RetrieveBuildingData(const tinyxml2::XMLElement *element, const std::map<int32_t, glm::dvec2> &nodeIDToLonLat, BuildingData &outBuildingData)
{
    const tinyxml2::XMLElement *nodeRefElement = element->FirstChildElement(WAY_NODE_ELEMENT_STR);
    while (nodeRefElement != nullptr)
    {
        int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
        if (nodeIDToLonLat.find(nodeId) != nodeIDToLonLat.end())
        {
            double lon = nodeIDToLonLat.at(nodeId).x;
            double lat = nodeIDToLonLat.at(nodeId).y;
            glm::dvec2 xy = GeometryUtils::LonLatToXY(lon, lat);
            outBuildingData.outline.push_back(xy);
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

    glm::dvec2 min = outBuildingData.outline[0];
    glm::dvec2 max = min;
    for (size_t i = 1; i < outBuildingData.outline.size(); ++i)
    {
        min.x = glm::min(min.x, outBuildingData.outline[i].x);
        min.y = glm::min(min.y, outBuildingData.outline[i].y);
        max.x = glm::max(max.x, outBuildingData.outline[i].x);
        max.y = glm::max(max.y, outBuildingData.outline[i].y);
    }

    outBuildingData.positionInChunk = (min + max) / 2.0;
    for (size_t i = 0; i < outBuildingData.outline.size(); ++i)
    {
        outBuildingData.outline[i] -= outBuildingData.positionInChunk;
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
bool OSMChunkDataSource::RetrieveHighwayData(const tinyxml2::XMLElement *element, const std::map<int32_t, glm::dvec2> &nodeIDToLonLat, HighwayData &outHighwayData)
{
    const tinyxml2::XMLElement *nodeRefElement = element->FirstChildElement(WAY_NODE_ELEMENT_STR);
    while (nodeRefElement != nullptr)
    {
        int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
        if (nodeIDToLonLat.find(nodeId) != nodeIDToLonLat.end())
        {
            double lon = nodeIDToLonLat.at(nodeId).x;
            double lat = nodeIDToLonLat.at(nodeId).y;
            glm::dvec2 xy = GeometryUtils::LonLatToXY(lon, lat);
            outHighwayData.points.push_back(xy);
        }
        nodeRefElement = nodeRefElement->NextSiblingElement(WAY_NODE_ELEMENT_STR);
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

    if (outHighwayData.points.size() == 0)
    {
        return false;
    }

    glm::dvec2 min = outHighwayData.points[0];
    glm::dvec2 max = min;
    for (size_t i = 1; i < outHighwayData.points.size(); i++)
    {
        min.x = glm::min(min.x, outHighwayData.points[i].x);
        min.y = glm::min(min.y, outHighwayData.points[i].y);
        max.x = glm::max(max.x, outHighwayData.points[i].x);
        max.y = glm::max(max.y, outHighwayData.points[i].y);
    }

    glm::dvec2 center = (min + max) / 2.0;
    for (size_t i = 0; i < outHighwayData.points.size(); i++)
    {
        outHighwayData.points[i] -= center;
    }
    outHighwayData.position = center;

    return true;
}

bool OSMChunkDataSource::HasChildTag(const tinyxml2::XMLElement *parent, const char *key)
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

const tinyxml2::XMLAttribute* OSMChunkDataSource::GetChildTagValue(const tinyxml2::XMLElement *parent, const char *key)
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
