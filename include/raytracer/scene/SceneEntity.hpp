#pragma once

#include <glm/glm.hpp>

#include "raytracer/Ray.hpp"
#include "raytracer/Color.hpp"

class SceneEntity
{
public:
    // TODO: Rotate
    SceneEntity(const glm::vec3& pos)
        : _pos(pos) {}

    virtual float intersection(const Ray& in, Ray& out, Color& color) = 0;

    void set_position(const glm::vec3& pos) { _pos = pos; }

    const glm::vec3& get_position() const { return _pos; }

    virtual void recalc() = 0;

protected:
    glm::vec3 _pos;
};
