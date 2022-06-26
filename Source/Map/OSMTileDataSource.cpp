#include "Map/OSMTileDataSource.hpp"

#include "Map/TileDataSource.hpp"
#include "Util/GeometryUtils.hpp"

#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <tinyxml2.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

// Networking includes
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
    std::string fileName = GetTileFilePath(tileIndex, zoomLevel);

    tinyxml2::XMLDocument document;
    if (document.LoadFile(fileName.c_str()) == tinyxml2::XML_SUCCESS)
    {
        outTileData.index = tileIndex;
        return RetrieveFromXML(document, outTileData);
    }

    tinyxml2::XMLDocument *downloaded = RetrieveFromServer(tileIndex, zoomLevel);
    if (downloaded != nullptr)
    {
        outTileData.index = tileIndex;
        return RetrieveFromXML(*downloaded, outTileData);
    }

    std::cerr << "[OSMTileDataSource] Cannot retrieve map " << fileName << std::endl;
    return false;
}

/**
 * @brief Queries whether there is a tile cache available for the specified tile index and zoom level
 * @param[in] tileIndex Tile index
 * @param[in] zoomLevel Zoom level
 * @return True if there is a tile cache available
 */
bool OSMTileDataSource::IsTileCacheAvailable(const glm::ivec2 &tileIndex, const int &zoomLevel)
{
    std::string fileName = GetTileFilePath(tileIndex, zoomLevel);
    std::ifstream file(fileName);
    return file.good();
}

/**
 * @brief Prefetches the tile data at the specified tile index and zoom level, and
 * caches the result locally.
 * @param[in] tileIndex Tile index of the tile to prefetch
 * @param[in] zoomLevel Zoom level
 * @return True if the operation was successful
 */
bool OSMTileDataSource::Prefetch(const glm::ivec2 &tileIndex, const int &zoomLevel)
{
    std::string fileName = GetTileFilePath(tileIndex, zoomLevel);

    std::ifstream file(fileName);
    if (!file.fail())
    {
        return true;
    }

    tinyxml2::XMLDocument *doc = RetrieveFromServer(tileIndex, zoomLevel);
    if (doc == nullptr)
    {
        return false;
    }

    doc->SaveFile(fileName.c_str());
    delete doc;

    return true;
}

/**
 * @brief Gets the tile cache file path for the specified tile index and zoom level
 * @param[in] tileIndex Tile index
 * @param[in] zoomLevel Zoom level
 * @return File path for the specified tile index and zoom level
 */
std::string OSMTileDataSource::GetTileFilePath(const glm::ivec2 &tileIndex, const int &zoomLevel)
{
    std::stringstream ss;
    ss << "Resources/map_" << zoomLevel << "-" << tileIndex.x << "-" << tileIndex.y << ".osm";
    return ss.str();
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
        else if (HasWaterData(wayElement))
        {
            outTileData.waterFeatures.emplace_back();
            if (!RetrieveWaterData(wayElement, nodeIDToLonLat, outTileData.waterFeatures.back()))
            {
                outTileData.waterFeatures.pop_back();
            }
        }

        wayElement = wayElement->NextSiblingElement(WAY_ELEMENT_STR);
    }

    return true;
}

/**
 * @brief Retrieves tile data from the server
 * @param[in] tileIndex Tile index
 * @param[in] zoomLevel Zoom level
 * @return XML document representing the tile data. Returns nullptr if the tile data cannot be retrieved from the server
 */
tinyxml2::XMLDocument* OSMTileDataSource::RetrieveFromServer(const glm::ivec2 &tileIndex, const int &zoomLevel)
{
    RectD tileBounds = GeometryUtils::GetLonLatBoundsFromTile(tileIndex.x, tileIndex.y, zoomLevel);

    std::string host = "overpass-api.de";
    double left = tileBounds.min.x;
    double bottom = tileBounds.min.y;
    double right = tileBounds.max.x;
    double top = tileBounds.max.y;
    std::stringstream requestSS;
    requestSS << "GET /api/interpreter?data=[bbox:" << bottom << "%2C" << left << "%2C" << top << "%2C" << right << "];(node;<;);out%20meta;\r\n";
    std::string request = requestSS.str();
    
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_CANONNAME;

    addrinfo *result = nullptr;
    int res = getaddrinfo(host.c_str(), nullptr, &hints, &result);
    if (res != 0)
    {
        std::cout << "Failed to get host info!" << std::endl;
        return nullptr;
    }

    sockaddr_in *temp = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    temp->sin_port = htons(80);

    int socketFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd == -1)
    {
        std::cout << "[OSMTileDataSource] Failed to create socket!" << std::endl;
        freeaddrinfo(result);
        return nullptr;
    }

    int tcpNoDelayOn = 1;
    setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, &tcpNoDelayOn, sizeof(int));

    std::cout << "[OSMTileDataSource] Trying to connect..." << std::endl;
    res = connect(socketFd, result->ai_addr, result->ai_addrlen);
    if (res == -1)
    {
        std::cout << "[OSMTileDataSource] Failed to connect!" << std::endl;
        shutdown(socketFd, SHUT_RDWR);
        close(socketFd);
        freeaddrinfo(result);
        return nullptr;
    }

    std::cout << "[OSMTileDataSource] Posting request..." << std::endl;
    size_t sz = request.size() + sizeof(char);
    if (write(socketFd, request.c_str(), sz) != sz)
    {
        std::cout << "[OSMTileDataSource] Failed to post request!" << std::endl;
        shutdown(socketFd, SHUT_RDWR);
        close(socketFd);
        freeaddrinfo(result);
        return nullptr;
    }

    std::stringstream ss;
    const int BUFFER_SIZE = 1000000;
    char buf[BUFFER_SIZE];
    while ((sz = read(socketFd, buf, BUFFER_SIZE - 1)) != 0)
    {
        ss.write(buf, sz);
        memset(buf, 0, sizeof(char) * BUFFER_SIZE);
    }
    std::cout << "[OSMTileDataSource] Closing socket..." << std::endl;

    shutdown(socketFd, SHUT_RDWR);
    close(socketFd);
    freeaddrinfo(result);
    std::cout << "[OSMTileDataSource] Socket closed!" << std::endl;

    std::string str = ss.str();
    tinyxml2::XMLDocument *ret = new tinyxml2::XMLDocument();
    if (ret->Parse(str.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cout << "[OSMTileDataSource] Failed to parse XML!" << std::endl;
        delete ret;
        return nullptr;
    }
    std::cout << "[OSMTileDataSource] XML Loaded!" << std::endl;
    return ret;
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

/**
 * @brief Retrieve water feature data from the given xml element
 * @param[in] element XML element object
 * @param[in] nodeIDToLonLat Map containing the mapping between a node ID and its lon/lat position
 * @param[out] outWaterData WaterFeatureData object that will contain the retrieved water feature data
 * @return True if the operation was successful.
 */
bool OSMTileDataSource::RetrieveWaterData(const tinyxml2::XMLElement *element, const std::map<int32_t, glm::dvec2> &nodeIDToLonLat, WaterFeatureData &outWaterData)
{
    const tinyxml2::XMLElement *nodeRefElement = element->FirstChildElement(WAY_NODE_ELEMENT_STR);
    while (nodeRefElement != nullptr)
    {
        int32_t nodeId = nodeRefElement->IntAttribute(WAY_NODE_REF_ATTRIBUTE_STR);
        if (nodeIDToLonLat.find(nodeId) != nodeIDToLonLat.end())
        {
            double lon = nodeIDToLonLat.at(nodeId).x;
            double lat = nodeIDToLonLat.at(nodeId).y;
            outWaterData.outline.emplace_back(lon, lat);
        }
        nodeRefElement = nodeRefElement->NextSiblingElement(WAY_NODE_ELEMENT_STR);
    }

    if (outWaterData.outline.size() == 0)
    {
        return false;
    }

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
 * @brief Checks whether the given xml element contains data for a water feature
 * @param[in] element XML element
 * @return True if the given XML element contains data for a water feature 
 */
bool OSMTileDataSource::HasWaterData(const tinyxml2::XMLElement *element)
{
    if (HasChildTag(element, WATER_KEY_STR))
    {
        return true;
    }

    const tinyxml2::XMLAttribute *naturalAttrib = GetChildTagValue(element, NATURAL_KEY_STR);
    if (naturalAttrib != nullptr)
    {
        return strcmp(naturalAttrib->Value(), NATURAL_WATER_VALUE_STR) == 0;
    }

    return false;
}
