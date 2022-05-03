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

            double minX, maxX, minY, maxY;

            const tinyxml2::XMLElement *nodeRefElement = wayElement->FirstChildElement(WAY_NODE_ELEMENT_STR);
            while (nodeRefElement != nullptr)
            {
                int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
                if (nodeIDToLonLat.find(nodeId) != nodeIDToLonLat.end())
                {
                    double lon = nodeIDToLonLat[nodeId].x;
                    double lat = nodeIDToLonLat[nodeId].y;
                    glm::dvec2 xy = GeometryUtils::LonLatToXY(lon, lat) * SCALE;

                    if (outChunkData.buildings.back().outline.size() == 0)
                    {
                        minX = maxX = xy.x;
                        minY = maxY = xy.y;
                    }
                    else
                    {
                        minX = glm::min(xy.x, minX);
                        maxX = glm::max(xy.x, maxX);
                        minY = glm::min(xy.y, minY);  
                        maxY = glm::max(xy.y, maxY);
                    }

                    outChunkData.buildings.back().outline.push_back(xy);
                }
                nodeRefElement = nodeRefElement->NextSiblingElement(WAY_NODE_ELEMENT_STR);
            }

            if (outChunkData.buildings.back().outline.size() == 0)
            {
                outChunkData.buildings.pop_back();
            }
            else
            {
                if (outChunkData.buildings.back().outline[0] == outChunkData.buildings.back().outline.back())
                {
                    outChunkData.buildings.back().outline.pop_back();
                }

                double centerX = (minX + maxX) / 2.0;
                double centerY = (minY + maxY) / 2.0;
                for (size_t i = 0; i < outChunkData.buildings.back().outline.size(); i++)
                {
                    outChunkData.buildings.back().outline[i].x -= centerX;
                    outChunkData.buildings.back().outline[i].y -= centerY;
                }

                outChunkData.buildings.back().position.x = centerX;
                outChunkData.buildings.back().position.y = centerY;

                const tinyxml2::XMLAttribute *heightAttrib = GetChildTagValue(wayElement, BUILDING_HEIGHT_TAG_KEY_STR);
                const tinyxml2::XMLAttribute *minHeightAttrib = GetChildTagValue(wayElement, BUILDING_MIN_HEIGHT_TAG_KEY_STR);
                const tinyxml2::XMLAttribute *buildingLevelsAttrib = GetChildTagValue(wayElement, BUILDING_LEVELS_TAG_KEY_STR);
                const tinyxml2::XMLAttribute *buildingMinLevelsAttrib = GetChildTagValue(wayElement, BUILDING_MIN_LEVELS_TAG_KEY_STR);
                // height has priority over building:levels
                if (heightAttrib != nullptr)
                {
                    outChunkData.buildings.back().heightInMeters = heightAttrib->DoubleValue();
                }
                else if (buildingLevelsAttrib != nullptr)
                {
                    outChunkData.buildings.back().heightInMeters = buildingLevelsAttrib->DoubleValue() * METERS_PER_LEVEL;
                }
                // min_height has priority over building:min_levels
                if (minHeightAttrib != nullptr)
                {
                    outChunkData.buildings.back().heightFromGround = minHeightAttrib->DoubleValue();
                    if (heightAttrib != nullptr)
                    {
                        outChunkData.buildings.back().heightInMeters -= minHeightAttrib->DoubleValue();
                    }
                }
                else if (buildingMinLevelsAttrib != nullptr)
                {
                    outChunkData.buildings.back().heightFromGround = buildingMinLevelsAttrib->DoubleValue() * METERS_PER_LEVEL;
                    if (heightAttrib != nullptr)
                    {
                        outChunkData.buildings.back().heightInMeters = glm::max(heightAttrib->DoubleValue() - outChunkData.buildings.back().heightFromGround, METERS_PER_LEVEL);
                    }
                    else if (buildingLevelsAttrib != nullptr)
                    {
                        outChunkData.buildings.back().heightInMeters -= outChunkData.buildings.back().heightFromGround;
                    }
                }
                outChunkData.buildings.back().heightInMeters *= SCALE;
                outChunkData.buildings.back().heightFromGround *= SCALE;
            }
        }
        else if (HasChildTag(wayElement, HIGHWAY_TAG_KEY_STR))
        {
            outChunkData.highways.emplace_back();
            const tinyxml2::XMLElement *nodeRefElement = wayElement->FirstChildElement(WAY_NODE_ELEMENT_STR);
            while (nodeRefElement != nullptr)
            {
                int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
                if (nodeIDToLonLat.find(nodeId) != nodeIDToLonLat.end())
                {
                    double lon = nodeIDToLonLat[nodeId].x;
                    double lat = nodeIDToLonLat[nodeId].y;
                    glm::dvec2 xy = GeometryUtils::LonLatToXY(lon, lat) * SCALE;
                    outChunkData.highways.back().points.push_back(xy);
                }
                nodeRefElement = nodeRefElement->NextSiblingElement(WAY_NODE_ELEMENT_STR);
            }

            double numLanes = 1.0;
            double width = RESIDENTIAL_HIGHWAY_LANE_WIDTH_METERS;
            const tinyxml2::XMLAttribute *highwayTypeAttrib = GetChildTagValue(wayElement, HIGHWAY_TAG_KEY_STR);
            if (highwayTypeAttrib != nullptr)
            {
                if (strcmp(highwayTypeAttrib->Value(), "primary") == 0)
                {
                    width = PRIMARY_HIGHWAY_LANE_WIDTH_METERS;
                }
            }
            const tinyxml2::XMLAttribute *lanesAttrib = GetChildTagValue(wayElement, HIGHWAY_LANES_TAG_KEY_STR);
            if (lanesAttrib != nullptr)
            {
                numLanes = lanesAttrib->DoubleValue();
            }
            outChunkData.highways.back().roadWidth = width * numLanes * SCALE;

            if (outChunkData.highways.back().points.size() == 0)
            {
                outChunkData.highways.pop_back();
            }
            else
            {
                glm::dvec2 min = outChunkData.highways.back().points[0];
                glm::dvec2 max = min;
                for (size_t i = 1; i < outChunkData.highways.back().points.size(); i++)
                {
                    min.x = glm::min(min.x, outChunkData.highways.back().points[i].x);
                    min.y = glm::min(min.y, outChunkData.highways.back().points[i].y);
                    max.x = glm::max(max.x, outChunkData.highways.back().points[i].x);
                    max.y = glm::max(max.y, outChunkData.highways.back().points[i].y);
                }

                glm::dvec2 center = (min + max) / 2.0;
                for (size_t i = 0; i < outChunkData.highways.back().points.size(); i++)
                {
                    outChunkData.highways.back().points[i] -= center;
                }
                outChunkData.highways.back().position = center;
            }
        }

        wayElement = wayElement->NextSiblingElement(WAY_ELEMENT_STR);
    }

    if (outChunkData.buildings.size() > 0)
    {
        double minX = outChunkData.buildings[0].position.x;
        double maxX = minX;
        double minY = outChunkData.buildings[0].position.y;
        double maxY = minY;
        for (size_t i = 1; i < outChunkData.buildings.size(); i++)
        {
            minX = glm::min(minX, outChunkData.buildings[i].position.x);
            maxX = glm::max(maxX, outChunkData.buildings[i].position.x);
            minY = glm::min(minY, outChunkData.buildings[i].position.y);
            maxY = glm::max(maxY, outChunkData.buildings[i].position.y);
        }

        outChunkData.position.x = (minX + maxX) * 0.5f;
        outChunkData.position.y = (minY + maxY) * 0.5f;
        for (size_t i = 0; i < outChunkData.buildings.size(); i++)
        {
            outChunkData.buildings[i].position -= outChunkData.position;
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
