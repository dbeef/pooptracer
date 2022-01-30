#pragma once

#include <cstdlib>

using Res_t = std::uint32_t;

struct Options
{
    static constexpr Res_t WIDTH = 320;
    static constexpr Res_t HEIGHT = 240;
    static constexpr std::size_t THREAD_COUNT = 16;
    static constexpr std::size_t RAYS_TOTAL = Options::WIDTH * Options::HEIGHT;
    static constexpr std::size_t RAYS_PER_THREAD = (static_cast<float>(Options::WIDTH * Options::HEIGHT) / THREAD_COUNT);
    static constexpr unsigned int SEED = 0;
};
