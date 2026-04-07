#include "FC3DArea.h"
#include <cmath>
#include <algorithm>

FC3DCell FC3DArea::at(int i, int j, int k) const noexcept
{
    if (i < 0 || i >= cellCountX() ||
        j < 0 || j >= cellCountY() ||
        k < 0 || k >= cellCountZ())
    {
        return FC3DCellError;
    }

    FC3DSize cs = cellSize();
    FC3DPoint anchor{
        _startPoint.x() + static_cast<float>(i) * cs.width(),
        _startPoint.y() + static_cast<float>(j) * cs.height(),
        _startPoint.z() + static_cast<float>(k) * cs.depth()
    };

    return FC3DCell{anchor, cs};
}

FC3DCell FC3DArea::at(const FC3DPoint& point) const noexcept
{
    if (!contains(point))
    {
        return FC3DCellError;
    }
    FC3DSize cs = cellSize();
    int i = static_cast<int>(std::floor((point.x() - _startPoint.x()) / cs.width()));
    int j = static_cast<int>(std::floor((point.y() - _startPoint.y()) / cs.height()));
    int k = static_cast<int>(std::floor((point.z() - _startPoint.z()) / cs.depth()));
    // Ограничение индексов на случай погрешностей округления на границах
    i = std::min(i, cellCountX() - 1);
    j = std::min(j, cellCountY() - 1);
    k = std::min(k, cellCountZ() - 1);
    return at(i, j, k);
}

FC3DPoint FC3DArea::indexAt(const FC3DPoint& point) const noexcept
{
    if(!contains(point))
    {
        return FC3DPointError;
    }
    FC3DSize cs = cellSize();
    int i = static_cast<int>(std::floor((point.x() - _startPoint.x()) / cs.width()));
    int j = static_cast<int>(std::floor((point.y() - _startPoint.y()) / cs.height()));
    int k = static_cast<int>(std::floor((point.z() - _startPoint.z()) / cs.depth()));
    // Ограничение индексов на случай погрешностей округления на границах
    i = std::min(i, cellCountX() - 1);
    j = std::min(j, cellCountY() - 1);
    k = std::min(k, cellCountZ() - 1);
    return FC3DPoint{
        static_cast<float>(i),
        static_cast<float>(j),
        static_cast<float>(k)
    };
}
