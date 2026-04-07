#include "FC3DPoint.h"
#include "FC3DSize.h"

constexpr FC3DPoint FC3DSize::toPoint() const noexcept { return FC3DPoint{_w, _h, _d}; }

constexpr FC3DSize FC3DSize::fromPoint(const FC3DPoint& p) noexcept { return FC3DSize{p.x(), p.y(), p.z()}; }
