#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct Color
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;

    std::size_t total() const { return r + g + b; };

    Color operator+(const Color& other) const
    {
        return Color{static_cast<uint8_t>(r + other.r), static_cast<uint8_t>(g + other.g), static_cast<uint8_t>(b + other.b)};
    }

    Color operator-(const Color& other) const
    {
        return Color{static_cast<uint8_t>(r + other.r), static_cast<uint8_t>(g + other.g), static_cast<uint8_t>(b + other.b)};
    }

    Color operator*(const float& v) const
    {
        return Color{static_cast<uint8_t>(r * v), static_cast<uint8_t>(g * v), static_cast<uint8_t>(b * v)};
    }

    static Color white() { return {255, 255, 255}; }
    static Color black() {  return {0, 0, 0}; }
    static Color red() { return {255, 0, 0}; }
    static Color blue() { return {0, 0, 255}; }
    static Color light_blue() { return {50, 50, 255}; }
    static Color green() { return {0, 255, 0}; }
};
