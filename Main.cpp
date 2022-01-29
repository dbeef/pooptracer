#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <stdio.h>
#include <stdbool.h>
#include <vector>
#include <iostream>
#include <memory>

#include <SDL.h>

#define WIN_WIDTH 320
#define WIN_HEIGHT 240

uint8_t pixels[WIN_WIDTH * WIN_HEIGHT * 4] = {0};

struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    std::size_t total() const { return r + g + b; };
};

static Color white = {255, 255, 255};
static Color black = {0, 0, 0};
static Color red = {255, 0, 0};
static Color blue= {0, 0, 255};
static Color green= {0, 255, 0};

Color* get_pixel(int n)
{
    return reinterpret_cast<Color*>(&pixels[4 * n]);
}

Color* get_pixel(int x, int y)
{
    std::size_t index = (y * WIN_WIDTH) + x;
    return get_pixel(index);
}

void set_pixel(int n, Color color)
{
    pixels[4 * n + 0] = color.r;
    pixels[4 * n + 1] = color.g;
    pixels[4 * n + 2] = color.b;
}

void set_pixel(int x, int y, Color color)
{
    std::size_t index = (y * WIN_WIDTH) + x;
    set_pixel(index, color);
}

struct Camera
{
    explicit Camera(glm::vec3 pos) : position(pos) {};
    Camera(){};

    static constexpr float f = 100;
    int w = WIN_WIDTH;
    int h = WIN_HEIGHT;
    glm::vec3 position;
};

struct Ray
{
    int x;
    int y;

    glm::vec3 position{};
    glm::vec3 direction{};
};

int dot(const glm::vec3& v1, const glm::vec3& v2)
{
    return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

glm::vec3 cross(const glm::vec3& v1, const glm::vec3& v2)
{
    glm::vec3 out;

    out.x = (v1.y * v2.z) - (v1.z * v2.y);
    out.y = -(v1.x * v2.z - v1.z * v2.x);
    out.z = (v1.x * v2.y) - (v1.y * v2.x);

    return out;
}

class SceneEntity
{
public:
    SceneEntity(const glm::vec3& pos, const Color& color) : _pos(pos), _color(color) {}
    virtual float intersection(const Ray&) = 0;
    void set_position(const glm::vec3& pos) { _pos = pos; }
    const glm::vec3& get_position() const { return _pos; }
    const Color& get_color() const { return _color; } 
protected:
    glm::vec3 _pos;
    Color _color;
};

class Sphere : public SceneEntity
{
public:
    Sphere(const glm::vec3& pos, float radius) : SceneEntity(pos, red), _radius_squared(radius * radius) { }
    float intersection(const Ray& r) override
    {
        float intersection_distance;
        const bool intersection = glm::intersectRaySphere(
            r.position,
            r.direction,
            _pos,
            _radius_squared,
            intersection_distance
        );

        return intersection ? intersection_distance : 0;
    }
private:
    const float _radius_squared;
};

class Plane : public SceneEntity
{
public:
    Plane(const glm::vec3& pos, const glm::vec3& normal) : SceneEntity(pos, blue), _normal(normal)
    {
    }

    float intersection(const Ray& r) override
    {
        float intersection_distance;
        const bool intersection = glm::intersectRayPlane(
            r.position, r.direction, _pos, _normal, intersection_distance
        );
        return intersection ? intersection_distance : 0;
    }
private:
    const glm::vec3 _normal;
};

struct
{
    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -1.0f));
    std::vector<Ray> rays;
    std::vector<std::shared_ptr<SceneEntity>> scene;
} raytracer;

void init_raytracer()
{
    // Generate rays:
    const auto half_height = raytracer.camera.h / 2.0f;
    const auto half_width = raytracer.camera.w / 2.0f;
    for (int y = 0; y < raytracer.camera.h; y++)
    {
        for (int x = 0; x < raytracer.camera.w; x++)
        {
            // Generate rays
            // Ray coming out of camera 'f' and crossing through the xy on matrix:
            Ray r{};

            r.x = x;
            r.y = y;

            // y
            // ^
            // |
            // z---> x 
            // x - left/right, y - forward/backward, z - up/down

            r.position = raytracer.camera.position;
           
            r.direction.x = x - half_width;
            r.direction.y = Camera::f;
            r.direction.z = y - half_height;

            r.direction = glm::normalize(r.direction);

            raytracer.rays.push_back(r);
        }
    }
    // Fill scene:
    const glm::vec3 plane_pos = {0, 0, 0};
    const glm::vec3 plane_normal = {0, 0 , 1};
    raytracer.scene.push_back(std::make_shared<Plane>(plane_pos, plane_normal));
    raytracer.scene.push_back(std::make_shared<Sphere>(glm::vec3{0, 4, -0.5f}, 1));
    raytracer.scene.push_back(std::make_shared<Sphere>(glm::vec3{2, 4, -0.5f}, 1));
    raytracer.scene.push_back(std::make_shared<Sphere>(glm::vec3{-2, 6, -0.5f}, 1));
}

void update_raytracer()
{
    memset(pixels, 0, WIN_WIDTH * WIN_HEIGHT * 4);

    auto& sphere = raytracer.scene.at(1);
    const auto& position = sphere->get_position();
    glm::vec3 new_pos = position;
    static float timer = 0;
    timer += 0.05f;

    new_pos.z -= std::sin(timer) / 10;

    sphere->set_position(new_pos);

    for (const auto& r : raytracer.rays)
    {
        float intersection_distance = 999999;
        std::shared_ptr<SceneEntity> collided_entity = nullptr;
        for (const auto& scene_entity : raytracer.scene)
        {
            const float id = scene_entity->intersection(r);
            if (id != 0 && id < intersection_distance)
            {
                intersection_distance = id;
                collided_entity = scene_entity;
            }
        }

        if (collided_entity == nullptr)
        { 
            set_pixel(r.x, r.y, black);
        } 
        else
        {
            const auto distance_modifier = (intersection_distance * 100 <= 255.0f ? intersection_distance * 100 : 255) / 255;
            const auto& color = collided_entity->get_color();
            auto color_distance_adjusted = color;
            color_distance_adjusted.r *= distance_modifier;
            color_distance_adjusted.g *= distance_modifier;
            color_distance_adjusted.b *= distance_modifier;
            set_pixel(r.x, r.y, color_distance_adjusted);
        } 
    }
} 
 
int main(int argc, char **argv) {
 
    init_raytracer();
 
    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }
 
    // create SDL window
    SDL_Window *window = SDL_CreateWindow("sdl2_pixelbuffer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIN_WIDTH * 4,
        WIN_HEIGHT * 4,
        SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return 1;
    }

    // create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return 1;
    }
    
    SDL_RenderSetLogicalSize(renderer, WIN_WIDTH, WIN_HEIGHT);

    // create texture
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        WIN_WIDTH,
        WIN_HEIGHT);
    if (texture == NULL) {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return 1;
    }

    set_pixel(0, {255, 0, 0});
    set_pixel(1, {0, 255, 0});
    set_pixel(2, {0, 0, 255});

    // main loop
    bool should_quit = false;
    SDL_Event e;
    while (!should_quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                should_quit = true;
            }
        }

        // update texture with new data
        int texture_pitch = 0;
        void* texture_pixels = NULL;
        if (SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch) != 0) {
            SDL_Log("Unable to lock texture: %s", SDL_GetError());
        }
        else {
            memcpy(texture_pixels, pixels, texture_pitch * WIN_HEIGHT);
        }
        SDL_UnlockTexture(texture);

        // render on screen
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        update_raytracer();
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

