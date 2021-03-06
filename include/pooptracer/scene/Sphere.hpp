#pragma once

#include <glm/glm.hpp>

class Sphere : public SceneEntity
{
public:
    void recalc() override{};

    Sphere(const glm::vec3& pos, float radius)
        : SceneEntity(pos), _radius_squared(radius * radius) {}

    float intersection(const Ray& in, Ray& out, Color& color) override
    {
        color = _color;

        float intersection_distance;
        const bool intersection = glm::intersectRaySphere(
            in.position,
            in.direction,
            _pos,
            _radius_squared,
            intersection_distance);

        const auto intersection_point = in.position + (in.direction * intersection_distance);

        const glm::vec3 normal = glm::normalize(intersection_point - _pos);

        out.direction = glm::normalize(normal + in.direction);
        out.position = intersection_point;

        return intersection ? intersection_distance : 0;
    }

private:
    static Color random_color()
    {
        switch (std::rand() % 4)
        {
            case 0: return Color::blue();
            case 1: return Color::green();
            case 2: return Color::red();
            case 3: return Color::yellow();
        }

        assert(false);
        return {};
    }

    const Color _color = random_color();
    const float _radius_squared;
};
