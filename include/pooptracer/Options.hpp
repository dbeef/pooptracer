#pragma once

#include <cstdlib>
#include <glm/glm.hpp>

using Res_t = std::uint32_t;

struct Options
{
    static constexpr Res_t WIDTH = 320;
    static constexpr Res_t HEIGHT = 240;
    static constexpr std::size_t THREAD_COUNT = 16;
    static constexpr std::size_t RAYS_TOTAL = Options::WIDTH * Options::HEIGHT;
    static constexpr std::size_t RAYS_PER_THREAD = (static_cast<float>(Options::WIDTH * Options::HEIGHT) / THREAD_COUNT);
    static constexpr float CAMERA_F = 100;
    static constexpr glm::vec3 CAMERA_POSITION = {0, 0, -1};
    static constexpr unsigned int SEED = 0;
};
