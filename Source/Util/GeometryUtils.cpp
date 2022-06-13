#include "Util/GeometryUtils.hpp"

#include "glm/ext/scalar_constants.hpp"

#include <cstdint>
#include <iostream>

const double EARTH_RADIUS = 6378137.0;

namespace GeometryUtils
{
bool IsCollinear(const glm::dvec2 &a, const glm::dvec2 &b, const glm::dvec2 &c)
{
	glm::dvec2 u = b - a;
	glm::dvec2 v = c - a;
	float winding = u.x * v.y - v.x * u.y;
	return glm::abs(winding) <= glm::epsilon<float>();
}

bool IsCCW(const glm::dvec2 &a, const glm::dvec2 &b, const glm::dvec2 &c)
{
	glm::dvec2 u = b - a;
	glm::dvec2 v = c - a;
	float winding = u.x * v.y - v.x * u.y;
	return winding > 0.0f;
}

bool IsPointInsideTriangle(const glm::dvec2& point, const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c)
{
	bool ccw_AP_AB = IsCCW(a, point, b);
	bool ccw_BP_BC = IsCCW(b, point, c);
	bool ccw_CP_CA = IsCCW(c, point, a);

	return (!ccw_AP_AB) && (!ccw_BP_BC) && (!ccw_CP_CA);
}

bool IsPolygonCCW(const std::vector<glm::dvec2> &polygonPoints)
{
	float sum = 0.0f;
	for (size_t i = 0; i < polygonPoints.size(); i++)
	{
		const glm::dvec2 &p0 = polygonPoints[i];
		const glm::dvec2 &p1 = polygonPoints[(i + 1) % polygonPoints.size()];

		sum += (p1.x - p0.x) * (p1.y + p0.y);
	}

	return (sum < 0.0f);
}

void PolygonTriangulation(const std::vector<glm::dvec2> &polygonPoints, std::vector<glm::dvec2> &outPoints)
{
    outPoints.clear();

    if (polygonPoints.size() < 3)
	{
		return;
	}

	std::vector<glm::dvec2> inputPointsCopy(polygonPoints.begin(), polygonPoints.end());
	while (inputPointsCopy.size() >= 3)
	{
		bool earFound = false;
		int32_t numPoints = static_cast<int32_t>(inputPointsCopy.size());
		for (int32_t i = 0; i < numPoints; ++i)
		{
			int32_t prevIndex = ((i - 1) + numPoints) % numPoints;
			int32_t currIndex = i;
			int32_t nextIndex = (i + 1) % numPoints;

			glm::dvec2 &prev = inputPointsCopy[prevIndex];
			glm::dvec2 &curr = inputPointsCopy[currIndex];
			glm::dvec2 &next = inputPointsCopy[nextIndex];

			bool isEar = false;
			if (IsCCW(prev, curr, next))
			{
				isEar = true;

				// Check if other points are inside the potential ear
				for (int32_t j = 0; j < numPoints; ++j)
				{
					// Make sure the point is not part of the potential ear
					if ((j == prevIndex) || (j == currIndex) || (j == nextIndex))
					{
						continue;
					}

					if (IsPointInsideTriangle(inputPointsCopy[j], prev, curr, next))
					{
						isEar = false;
						break;
					}
				}
			}

			if (isEar)
			{
				earFound = true;
				outPoints.push_back(prev);
                outPoints.push_back(curr);
                outPoints.push_back(next);
				inputPointsCopy.erase(inputPointsCopy.begin() + currIndex);
				break;
			}
		}

		if (!earFound)
		{
			break;
		}
	}
}

/**
 * @brief Converts the provided longitude-latitude coordinates to cartesian coordinates
 * @param[in] lonlat Longitude-latitude coordinates
 * @return Vector containing the corresponding cartesian coordinates (in meters)
 */
glm::dvec2 LonLatToXY(const glm::dvec2 &lonLat)
{
	return LonLatToXY(lonLat.x, lonLat.y);
}

/**
 * @brief Converts the provided longitude-latitude coordinates to cartesian coordinates
 * @param[in] lon Longitude
 * @param[in] lat Latitude
 * @return Vector containing the corresponding cartesian coordinates (in meters)
 */
glm::dvec2 LonLatToXY(const double &lon, const double &lat)
{
	glm::dvec2 ret;
	ret.x = glm::radians(lon) * EARTH_RADIUS;
    ret.y = glm::log(glm::tan(glm::radians(lat) / 2.0 + glm::pi<double>() / 4.0)) * EARTH_RADIUS;
	return ret;
}

glm::dvec2 XYToLonLat(const double x, const double y)
{
	glm::dvec2 ret;
	ret.x = glm::degrees(x / EARTH_RADIUS);

	double pi = glm::pi<double>();
	ret.y = glm::degrees(2 * glm::atan(glm::exp(y / EARTH_RADIUS)) - pi / 2.0);

	return ret;
}

/**
 * @brief Converts the provided longitude-latitude coordinates to its corresponding tile index
 * @param[in] lon Longitude
 * @param[in] lat Latitude
 * @param[in] zoomLevel Zoom level
 * @return Tile index of the tile that contains the provided longitude-latitude
 */
glm::ivec2 LonLatToTileIndex(const double &lon, const double &lat, const int &zoomLevel)
{
	glm::ivec2 ret;
	ret.x = static_cast<int>(std::floor((lon + 180.0) / 360.0 * (1 << zoomLevel)));

	double latRadians = glm::radians(lat);
	double pi = glm::pi<double>();
	ret.y = static_cast<int>(std::floor((1.0 - std::asinh(std::tan(latRadians)) / pi) / 2.0 * (1 << zoomLevel)));

	return ret;
}

/**
 * @brief Converts the provided tile index to its corresponding longitude-latitude coordinates
 * @param[in] tileX Tile index along the x-axis
 * @param[in] tileY Tile index along the y-axis
 * @param[in] zoomLevel Zoom level
 * @return Longitude-latitude coordinates of the upper-left corner of the tile
 */
glm::dvec2 TileIndexToLonLat(const int &tileX, const int &tileY, const int &zoomLevel)
{
	glm::dvec2 ret;
	ret.x = tileX / static_cast<double>(1 << zoomLevel) * 360.0 - 180.0;

	double pi = glm::pi<double>();
	double n = pi - 2.0 * pi * tileY / static_cast<double>(1 << zoomLevel);
	ret.y = 180.0 / pi * std::atan(0.5 * (std::exp(n) - std::exp(-n)));

	return ret;
}

/**
 * @brief Gets the bounding box (in lon-lat) of the tile identified by the provided tile index
 * @param[in] tileX Tile index in the x-axis
 * @param[in] tileY Tile index in the y-axis
 * @param[in] zoomLevel Zoom level
 * @return Bounding box (in lon-lat) of the tile
 */
RectD GetLonLatBoundsFromTile(const int &tileX, const int &tileY, const int &zoomLevel)
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
}