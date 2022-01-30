#pragma once

#include "raytracer/Color.hpp"

class Framebuffer
{
public:
    using Res_t = std::uint32_t;

    static constexpr Res_t WIDTH = 320;
    static constexpr Res_t HEIGHT = 240;

    Color& get_pixel(Res_t n)
    {
        return pixels[n];
    }

    Color& get_pixel(Res_t x, Res_t y)
    {
        return get_pixel((y * WIDTH) + x);
    }

    void set_pixel(Res_t n, Color color)
    {
        get_pixel(n) = color;
    }

    void set_pixel(Res_t x, Res_t y, Color color)
    {
        set_pixel((y * WIDTH) + x, color);
    }

    const char* data() const
    {
        return reinterpret_cast<const char*>(&pixels);
    }

private:
    Color pixels[WIDTH * HEIGHT] = {0};
};
