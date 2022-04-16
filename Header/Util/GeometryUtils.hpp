#ifndef GEOMETRY_UTILS_HEADER
#define GEOMETRY_UTILS_HEADER

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace GeometryUtils
{
extern bool IsCollinear(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c);
extern bool IsCCW(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c);
extern bool IsPointInsideTriangle(const glm::vec2& point, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c);
extern bool IsPolygonCCW(const std::vector<glm::vec2> &polygonPoints);
extern void PolygonTriangulation(const std::vector<glm::vec2> &polygonPoints, std::vector<glm::vec2> &outPoints);
}

#endif // GEOMETRY_UTILS_HEADER
