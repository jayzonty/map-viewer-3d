#ifndef GEOMETRY_UTILS_HEADER
#define GEOMETRY_UTILS_HEADER

#include "Core/Rect.hpp"
#include "glm/fwd.hpp"
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace GeometryUtils
{
extern bool IsCollinear(const glm::dvec2 &a, const glm::dvec2 &b, const glm::dvec2 &c);
extern bool IsCCW(const glm::dvec2 &a, const glm::dvec2 &b, const glm::dvec2 &c);
extern bool IsPointInsideTriangle(const glm::dvec2& point, const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);
extern bool IsPolygonCCW(const std::vector<glm::dvec2> &polygonPoints);
extern void PolygonTriangulation(const std::vector<glm::dvec2> &polygonPoints, std::vector<glm::dvec2> &outPoints);

/**
 * @brief Converts the provided longitude-latitude coordinates to cartesian coordinates
 * @param[in] lonlat Longitude-latitude coordinates
 * @return Vector containing the corresponding cartesian coordinates (in meters)
 */
extern glm::dvec2 LonLatToXY(const glm::dvec2 &lonLat);

/**
 * @brief Converts the provided longitude-latitude coordinates to cartesian coordinates
 * @param[in] lon Longitude
 * @param[in] lat Latitude
 * @return Vector containing the corresponding cartesian coordinates (in meters)
 */
extern glm::dvec2 LonLatToXY(const double &lon, const double &lat);

extern glm::dvec2 XYToLonLat(const double x, const double y);

/**
 * @brief Converts the provided longitude-latitude coordinates to its corresponding tile index
 * @param[in] lon Longitude
 * @param[in] lat Latitude
 * @param[in] zoomLevel Zoom level
 * @return Tile index of the tile that contains the provided longitude-latitude
 */
extern glm::ivec2 LonLatToTileIndex(const double &lon, const double &lat, const int &zoomLevel);

/**
 * @brief Converts the provided tile index to its corresponding longitude-latitude coordinates
 * @param[in] tileX Tile index along the x-axis
 * @param[in] tileY Tile index along the y-axis
 * @param[in] zoomLevel Zoom level
 * @return Longitude-latitude coordinates of the upper-left corner of the tile
 */
extern glm::dvec2 TileIndexToLonLat(const int &tileX, const int &tileY, const int &zoomLevel);

/**
 * @brief Gets the bounding box (in lon-lat) of the tile identified by the provided tile index
 * @param[in] tileX Tile index in the x-axis
 * @param[in] tileY Tile index in the y-axis
 * @param[in] zoomLevel Zoom level
 * @return Bounding box (in lon-lat) of the tile
 */
extern RectD GetLonLatBoundsFromTile(const int &tileX, const int &tileY, const int &zoomLevel);
}

#endif // GEOMETRY_UTILS_HEADER
