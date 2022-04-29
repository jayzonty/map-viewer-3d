#include "Util/GeometryUtils.hpp"

#include "glm/ext/scalar_constants.hpp"

#include <cstdint>
#include <iostream>

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
}