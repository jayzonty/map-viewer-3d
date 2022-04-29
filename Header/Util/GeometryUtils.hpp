#ifndef GEOMETRY_UTILS_HEADER
#define GEOMETRY_UTILS_HEADER

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
}

#endif // GEOMETRY_UTILS_HEADER
