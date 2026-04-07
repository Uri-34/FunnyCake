#ifndef FC_CELSIUS_FAHRENHEIT_H
#define FC_CELSIUS_FAHRENHEIT_H

#include "qglobal.h"

///< перевод из градусов цельсия в градусы фаренгейта
constexpr qreal toFahrenheit(qreal celsius) { return (celsius * 9.0 / 5.0) + 32.0; }
///< перевод из градусов фаренгейта в градусы цельсия
constexpr qreal toCelsius(qreal fahrenheit) { return (fahrenheit - 32.0) * 5.0 / 9.0; }

#endif // FC_CELSIUS_FAHRENHEIT_H
