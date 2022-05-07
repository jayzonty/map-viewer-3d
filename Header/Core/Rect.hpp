#ifndef RECT_HEADER
#define RECT_HEADER

#include <glm/detail/qualifier.hpp>

/**
 * Struct representing a bounding box
 */
template <typename T>
struct Rect
{
    glm::vec<2, T> min;     // Min point
    glm::vec<2, T> max;     // Max point

    /**
     * @brief Constructs a Rect object from a center and its half-extents (or radius)
     * @param[in] center Center
     * @param[in] radius Radius (or half-extents)
     * @return Constructed Rect object
     */
    static Rect FromCenterRadius(const glm::vec<2, T> &center, const glm::vec<2, T> &radius)
    {
        return { center - radius, center + radius };
    }

    /**
     * @brief Queries whether the two bounding boxes are overlapping or not
     * @param[in] a First bounding box
     * @param[in] b Second bounding box
     * @return True if the two bounding boxes are overlapping.
     */
    static bool IsOverlapping(const Rect<T> &a, const Rect<T> &b)
    {
        for (size_t i = 0; i < 2; ++i)
        {
            if ((a.min[i] > b.max[i]) || (b.min[i] > a.max[i]))
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Queries whether the specified pont is inside the specified bounding box
     * @param[in] a Bounding box
     * @param[in] point Point
     * @return True if point is insdie the bounding box.
     */
    static bool IsPointInsideRect(const Rect<T> &a, const glm::vec<2, T> &point)
    {
        for (size_t i = 0; i < 2; ++i)
        {
            if ((point[i] < a.min[i]) || (point[i] > a.max[i]))
            {
                return false;
            }
        }
        return true;
    }
};

typedef Rect<float> RectF;      // Float-based bounding box
typedef Rect<double> RectD;     // Double-based bounding box
typedef Rect<int32_t> RectI;    // Int32-based bounding box

#endif // RECT_HEADER
