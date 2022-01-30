#pragma once

#include "raytracer/Color.hpp"
#include "raytracer/Options.hpp"

class Framebuffer
{
public:
    Color& get_pixel(Res_t n)
    {
        return pixels[n];
    }

    Color& get_pixel(Res_t x, Res_t y)
    {
        return get_pixel((y * Options::WIDTH) + x);
    }

    void set_pixel(Res_t n, Color color)
    {
        get_pixel(n) = color;
    }

    void set_pixel(Res_t x, Res_t y, Color color)
    {
        set_pixel((y * Options::WIDTH) + x, color);
    }

    const char* data() const
    {
        return reinterpret_cast<const char*>(&pixels);
    }

private:
    Color pixels[Options::WIDTH * Options::HEIGHT];
};
