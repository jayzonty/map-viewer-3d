#include "Map/MapData.hpp"
#include "Util/GeometryUtils.hpp"
#include "glm/ext/scalar_constants.hpp"

#include <algorithm>
#include <limits>
#include <tinyxml2.h>

#include <cstring>
#include <iostream>
#include <map>

MapData::MapData()
{
}

MapData::~MapData()
{
}

bool MapData::Parse(const std::string &mapFilePath)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(mapFilePath.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cerr << "[Main] Failed to load " << mapFilePath << std::endl;
        return false;
    }

    std::map<int32_t, glm::vec2> nodeIdToLatLong;

    tinyxml2::XMLElement *nodeElement = doc.FirstChildElement(OSM_ELEMENT_STR)->FirstChildElement(NODE_ELEMENT_STR);
    while (nodeElement != nullptr)
    {
        int32_t nodeId = nodeElement->IntAttribute(NODE_ID_ATTRIBUTE_STR);
        if (nodeId != 0) // TODO: Not sure if 0 is a valid node ID
        {
            float lon = nodeElement->FloatAttribute(NODE_LON_ATTRIBUTE_STR);
            float lat = nodeElement->FloatAttribute(NODE_LAT_ATTRIBUTE_STR);
            nodeIdToLatLong[nodeId] = glm::vec2(lon, lat);
        }
        nodeElement = nodeElement->NextSiblingElement(NODE_ELEMENT_STR);
    }

    const double EARTH_RADIUS = 6378137.0;

    tinyxml2::XMLElement *wayElement = doc.FirstChildElement(OSM_ELEMENT_STR)->FirstChildElement(WAY_ELEMENT_STR);
    while (wayElement != nullptr)
    {
        if (HasChildTag(wayElement, BUILDING_TAG_KEY_STR)
            || HasChildTag(wayElement, BUILDING_PART_TAG_KEY_STR))
        {
            m_buildings.emplace_back();

            double minX, maxX, minY, maxY;

            tinyxml2::XMLElement *nodeRefElement = wayElement->FirstChildElement(WAY_NODE_ELEMENT_STR);
            while (nodeRefElement != nullptr)
            {
                int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
                if (nodeIdToLatLong.find(nodeId) != nodeIdToLatLong.end())
                {
                    double lon = nodeIdToLatLong[nodeId].x;
                    double lat = nodeIdToLatLong[nodeId].y;

                    double x = glm::radians(lon) * EARTH_RADIUS * SCALE;
                    double y = glm::log(glm::tan(glm::radians(lat) / 2.0 + glm::pi<double>() / 4.0)) * EARTH_RADIUS * SCALE;

                    if (m_buildings.back().outline.size() == 0)
                    {
                        minX = maxX = x;
                        minY = maxY = y;
                    }
                    else
                    {
                        minX = glm::min(x, minX);
                        maxX = glm::max(x, maxX);
                        minY = glm::min(y, minY);  
                        maxY = glm::max(y, maxY);
                    }

                    m_buildings.back().outline.push_back(glm::vec2(x, y));
                }
                nodeRefElement = nodeRefElement->NextSiblingElement(WAY_NODE_ELEMENT_STR);
            }

            if (m_buildings.back().outline.size() == 0)
            {
                m_buildings.pop_back();
            }
            else
            {
                if (m_buildings.back().outline[0] == m_buildings.back().outline.back())
                {
                    m_buildings.back().outline.pop_back();
                }

                double centerX = (minX + maxX) / 2.0;
                double centerY = (minY + maxY) / 2.0;
                for (size_t i = 0; i < m_buildings.back().outline.size(); i++)
                {
                    m_buildings.back().outline[i].x -= centerX;
                    m_buildings.back().outline[i].y -= centerY;
                }

                m_buildings.back().position.x = centerX;
                m_buildings.back().position.y = centerY;

                const tinyxml2::XMLAttribute *heightAttrib = GetChildTagValue(wayElement, BUILDING_HEIGHT_TAG_KEY_STR);
                const tinyxml2::XMLAttribute *minHeightAttrib = GetChildTagValue(wayElement, BUILDING_MIN_HEIGHT_TAG_KEY_STR);
                const tinyxml2::XMLAttribute *buildingLevelsAttrib = GetChildTagValue(wayElement, BUILDING_LEVELS_TAG_KEY_STR);
                const tinyxml2::XMLAttribute *buildingMinLevelsAttrib = GetChildTagValue(wayElement, BUILDING_MIN_LEVELS_TAG_KEY_STR);
                // height has priority over building:levels
                if (heightAttrib != nullptr)
                {
                    m_buildings.back().heightInMeters = heightAttrib->DoubleValue();
                }
                else if (buildingLevelsAttrib != nullptr)
                {
                    m_buildings.back().heightInMeters = buildingLevelsAttrib->DoubleValue() * METERS_PER_LEVEL;
                }
                // min_height has priority over building:min_levels
                if (minHeightAttrib != nullptr)
                {
                    m_buildings.back().heightFromGround = minHeightAttrib->DoubleValue();
                    if (heightAttrib != nullptr)
                    {
                        m_buildings.back().heightInMeters -= minHeightAttrib->DoubleValue();
                    }
                }
                else if (buildingMinLevelsAttrib != nullptr)
                {
                    m_buildings.back().heightFromGround = buildingMinLevelsAttrib->DoubleValue() * METERS_PER_LEVEL;
                    if (heightAttrib != nullptr)
                    {
                        m_buildings.back().heightInMeters = glm::max(heightAttrib->DoubleValue() - m_buildings.back().heightFromGround, METERS_PER_LEVEL);
                    }
                    else if (buildingLevelsAttrib != nullptr)
                    {
                        m_buildings.back().heightInMeters -= m_buildings.back().heightFromGround;
                    }
                }
                m_buildings.back().heightInMeters *= SCALE;
                m_buildings.back().heightFromGround *= SCALE;
            }
        }

        wayElement = wayElement->NextSiblingElement(WAY_ELEMENT_STR);
    }

    if (m_buildings.size() > 0)
    {
        float minX = m_buildings[0].position.x;
        float maxX = minX;
        float minY = m_buildings[0].position.y;
        float maxY = minY;
        for (size_t i = 1; i < m_buildings.size(); i++)
        {
            minX = glm::min(minX, m_buildings[i].position.x);
            maxX = glm::max(maxX, m_buildings[i].position.x);
            minY = glm::min(minY, m_buildings[i].position.y);
            maxY = glm::max(maxY, m_buildings[i].position.y);
        }

        m_position.x = (minX + maxX) * 0.5f;
        m_position.y = (minY + maxY) * 0.5f;
        for (size_t i = 0; i < m_buildings.size(); i++)
        {
            m_buildings[i].position -= m_position;
            for (size_t j = 0; j < m_buildings[i].outline.size(); j++)
            {
                glm::vec2 &a = m_buildings[i].outline[j];
                glm::vec2 &b = m_buildings[i].outline[(j + 1) % m_buildings[i].outline.size()];
                glm::vec2 &c = m_buildings[i].outline[(j + 2) % m_buildings[i].outline.size()];

                if (GeometryUtils::IsCollinear(a, b, c))
                {
                    m_buildings[i].outline.erase(m_buildings[i].outline.begin() + ((j + 1) % m_buildings[i].outline.size()));
                    --j;
                }
            }

            if (!GeometryUtils::IsPolygonCCW(m_buildings[i].outline))
            {
                std::reverse(m_buildings[i].outline.begin(), m_buildings[i].outline.end());
            }
            // Double check if polygon is now indeed CCW
            if (!GeometryUtils::IsPolygonCCW(m_buildings[i].outline))
            {
                std::cout << "Polygon is still not CCW!" << std::endl;
            }
        }
    }
    

    std::cout << "Num buildings: " << m_buildings.size() << std::endl;

    return true;
}

bool MapData::HasChildTag(tinyxml2::XMLElement *parent, const char *key)
{
    tinyxml2::XMLElement *curr = parent->FirstChildElement(TAG_ELEMENT_STR);
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

const tinyxml2::XMLAttribute* MapData::GetChildTagValue(tinyxml2::XMLElement *parent, const char *key)
{
    tinyxml2::XMLElement *curr = parent->FirstChildElement(TAG_ELEMENT_STR);
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
