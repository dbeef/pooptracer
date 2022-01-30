#pragma once

#include <glm/glm.hpp>

class Triangle : public SceneEntity
{
public:
    Triangle(const glm::vec3 origin, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
        : _v1(v1), _v2(v2), _v3(v3), SceneEntity(origin)
    {
        // TODO: Calc normal
        // TODO: Calc normal in plane
        // TODO: Function to change pos / rotate
        recalc();
    }

    void recalc() override
    {
        _v1_translated = _v1 + _pos;
        _v2_translated = _v2 + _pos;
        _v3_translated = _v3 + _pos;
        _normal = glm::cross(_v1_translated, _v2_translated);
    };

    float intersection(const Ray& in, Ray& out, Color& color) override
    {
        color = Color::light_blue();

        glm::vec2 bary;

        float intersection_distance;
        const bool intersection = glm::intersectRayTriangle(
            in.position, in.direction, _v1_translated, _v2_translated, _v3_translated, bary, intersection_distance);

        const auto intersection_point = in.position + (in.direction * intersection_distance);
        out.direction = glm::normalize(_normal + in.direction);
        out.position = intersection_point;

        return intersection ? intersection_distance : 0;
    }

private:
    glm::vec3 _v1;
    glm::vec3 _v2;
    glm::vec3 _v3;
    glm::vec3 _v1_translated;
    glm::vec3 _v2_translated;
    glm::vec3 _v3_translated;
    glm::vec3 _normal;
};
