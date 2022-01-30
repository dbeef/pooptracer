#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct Color
{
    constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 0)
        : r(r), g(b), b(b), a(a)
    {
    }

    Color()
    {
    }

    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{};

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

    static constexpr Color white() { return {255, 255, 255, 0}; }
    static constexpr Color black() { return {0, 0, 0, 0}; }
    static constexpr Color red() { return {255, 0, 0, 0}; }
    static constexpr Color blue() { return {0, 0, 255, 0}; }
    static constexpr Color light_blue() { return {50, 50, 255, 0}; }
    static constexpr Color green() { return {0, 255, 0, 0}; }
};
