#include "FC2DPoint.h"
#include "FC2DSize.h"
#include "FC3DPoint.h"

/// Конструктор из 3D точки (игнорируем z).
constexpr FC2DPoint::FC2DPoint(FC3DPoint& point) noexcept
    : _x(point.x()),
      _y(point.y())
{}

[[nodiscard]] constexpr FC3DPoint FC2DPoint::to3D(_type z) const noexcept { return FC3DPoint{_x, _y, z}; }

[[nodiscard]] constexpr FC2DPoint FC2DPoint::from2DSize(FC2DSize& size) noexcept { return FC2DPoint{size.width(), size.height()}; }


