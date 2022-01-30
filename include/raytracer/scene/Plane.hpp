#pragma once

#include <glm/glm.hpp>
#include "raytracer/scene/SceneEntity.hpp"

class Plane : public SceneEntity
{
public:
    void recalc() override{};

    Plane(const glm::vec3& pos, const glm::vec3& normal)
        : SceneEntity(pos), _normal(normal)
    {
    }

    float intersection(const Ray& in, Ray& out, Color& color) override
    {
        float intersection_distance;
        const bool intersection = glm::intersectRayPlane(
            in.position, in.direction, _pos, _normal, intersection_distance);

        const auto intersection_point = in.position + (in.direction * intersection_distance);
        out.direction = glm::normalize(_normal + in.direction);
        out.position = intersection_point;
        out.direction.z *= -1;

        if (static_cast<int>(intersection_point.x) % 2)
        {
            color = Color::white();
        }
        else
        {
            color = Color::black();
        }

        return intersection ? intersection_distance : 0;
    }

private:
    const glm::vec3 _normal;
};
