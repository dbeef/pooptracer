#pragma once

#include <glm/glm.hpp>

struct Ray
{
    int x;
    int y;

    glm::vec3 position{};
    glm::vec3 direction{};
};
