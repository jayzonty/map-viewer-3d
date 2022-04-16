#include <iostream>

#include "Application.hpp"

#include "Util/GeometryUtils.hpp"
#include "glm/fwd.hpp"

void evalute(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c)
{
    if (GeometryUtils::IsCollinear(a, b, c))
    {
        std::cout << "a, b, c are collinear" << std::endl;
    }
    else if (GeometryUtils::IsCCW(a, b, c))
    {
        std::cout << "a, b, c are CCW" << std::endl;
    }
    else 
    {
        std::cout << "a, b, c are CW" << std::endl;
    }

    if (GeometryUtils::IsPolygonCCW({a, b, c}))
    {
        std::cout << "Polygon is CCW" << std::endl;
    }
    else
    {
        std::cout << "Polygon is not CCW" << std::endl;
    }
}

int main()
{
    evalute({-0.5f, 0.5f}, {0.0f, 0.0f}, {0.5f, 0.5f});
    evalute({-0.5f, 0.5f}, {0.0f, 0.0f}, {-0.5f, -0.5f});
    evalute({-0.5f, 0.0f}, {0.0f, 0.5f}, {0.5f, 0.0f});

    std::vector<glm::vec2> poly =
    {
        { -0.5f, 0.5f },
        { 0.0f, 0.0f },
        { -0.5f, -0.5f }
    };
    if (!GeometryUtils::IsPolygonCCW(poly))
    {
        std::cout << "Reversing polygon..." << std::endl;
        std::reverse(poly.begin(), poly.end());
        if (!GeometryUtils::IsPolygonCCW(poly))
        {
            std::cout << "Polygon still not CCW..." << std::endl;
        }
    }
    //return 0;
    {
        Application app;
        app.Run();
    }

    return 0;
}
