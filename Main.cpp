#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <stdio.h>
#include <stdbool.h>
#include <vector>
#include <iostream>
#include <memory>
#include <random>

#include <SDL.h>

#define WIN_WIDTH 320
#define WIN_HEIGHT 240

// TODO: Optimize, multiple threads
// TODO: Cleanup + CMake project
// TODO: Add rotating triangle
// TODO: Benchmark project
// TODO: Lambda defining transformation

uint8_t pixels[WIN_WIDTH * WIN_HEIGHT * 4] = {0};

struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    std::size_t total() const { return r + g + b; };
    Color operator+(const Color& other) { return Color{r + other.r, g + other.g, b + other.b}; }
    Color operator-(const Color& other) { return Color{r + other.r, g + other.g, b + other.b}; }
    Color operator*(const float& v) { return Color{r * v, g * v, b * v}; }
};

static Color white = {255, 255, 255};
static Color black = {0, 0, 0};
static Color red = {255, 0, 0};
static Color blue= {0, 0, 255};
static Color light_blue= {50, 50, 255};
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

class SceneEntity
{
public:
    // TODO: Rotate
    SceneEntity(const glm::vec3& pos) : _pos(pos) {}
    virtual float intersection(const Ray& in, Ray& out, Color& color) = 0;
    void set_position(const glm::vec3& pos) { _pos = pos; }
    const glm::vec3& get_position() const { return _pos; }
    virtual void recalc() = 0;
protected:
    glm::vec3 _pos;
};

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
        color = light_blue;

        glm::vec2 bary;

        float intersection_distance;
        const bool intersection = glm::intersectRayTriangle(
            in.position, in.direction, _v1_translated, _v2_translated, _v3_translated, bary, intersection_distance
        );

        const auto intersection_point = in.position + (in.direction * intersection_distance);
        out.direction = glm::normalize(_normal + in.direction);
        out.position = intersection_point;
        out.direction.z *= -1;

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

class Sphere : public SceneEntity
{
public:

    void recalc() override {};
    Sphere(const glm::vec3& pos, float radius) : SceneEntity(pos), _radius_squared(radius * radius) { }
    float intersection(const Ray& in, Ray& out, Color& color) override
    {
        color = _color;

        float intersection_distance;
        const bool intersection = glm::intersectRaySphere(
            in.position,
            in.direction,
            _pos,
            _radius_squared,
            intersection_distance
        );

        const auto intersection_point = in.position + (in.direction * intersection_distance);

        const glm::vec3 normal = glm::normalize(intersection_point - _pos);

        out.direction = glm::normalize(normal + in.direction);
        out.position = intersection_point;

        return intersection ? intersection_distance : 0;
    }
private:
    static Color random_color()
    {
        return Color{std::rand() % 255, std::rand() % 255, std::rand() % 255};
    }

    const Color _color = random_color();
    const float _radius_squared;
};

class Plane : public SceneEntity
{
public:

    void recalc() override {};
    Plane(const glm::vec3& pos, const glm::vec3& normal) : SceneEntity(pos), _normal(normal)
    {
    }

    float intersection(const Ray& in, Ray& out, Color& color) override
    {
        float intersection_distance;
        const bool intersection = glm::intersectRayPlane(
            in.position, in.direction, _pos, _normal, intersection_distance
        );

        const auto intersection_point = in.position + (in.direction * intersection_distance);
        out.direction = glm::normalize(_normal + in.direction);
        out.position = intersection_point;
        out.direction.z *= -1;

        if (static_cast<int>(intersection_point.x) % 2)
        {
            color = white;
        }
        else
        {
            color = black;
        }

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
    std::srand(0);

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
    // raytracer.scene.push_back(std::make_shared<Sphere>(glm::vec3{2, 4, -0.5f}, 1));
    // raytracer.scene.push_back(std::make_shared<Sphere>(glm::vec3{-2, 6, -0.5f}, 1));
    raytracer.scene.push_back(std::make_shared<Triangle>(glm::vec3{1, 2, -1.5f}, glm::vec3{1, 0, 0}, glm::vec3{0, 0, 0}, glm::vec3{0, 0, -1}));
}

void update_raytracer()
{
    for (int i = 1; i < 3; i++)
    {
    auto& sphere = raytracer.scene.at(i);
    const auto& position = sphere->get_position();
    glm::vec3 new_pos = position;
    static float timer = 0;
    timer += 0.05f;

    new_pos.y += std::sin(timer) / 10;
    new_pos.x += std::cos(timer) / 10;

    sphere->set_position(new_pos);

    sphere->recalc();
    }

    for (const auto& r : raytracer.rays)
    {
        Color color;
        Ray out;
        float intersection_distance = 999999;
        std::shared_ptr<SceneEntity> collided_entity = nullptr;
        for (const auto& scene_entity : raytracer.scene)
        {
            Color temp_c;
            Ray temp_out;
            const float id = scene_entity->intersection(r, temp_out, temp_c);
            if (id != 0 && id < intersection_distance)
            {
                intersection_distance = id;
                collided_entity = scene_entity;
                color = temp_c;
                out = temp_out;
            }
        }

        if (collided_entity == nullptr)
        { 
            set_pixel(r.x, r.y, black);
        } 
        else
        {
            bool second_reflection = false;
            Color second_color = {0, 0, 0};
            for (const auto& scene_entity : raytracer.scene)
            {
                if (collided_entity == scene_entity)
                {
                    continue;
                }

                Color temp_c;
                Ray temp_out;
                const float id = scene_entity->intersection(out, temp_out, temp_c);
                if (id != 0 && id < intersection_distance)
                {
                    second_color = temp_c;
                    second_reflection = true;
                }
            }

            const auto out_color = (color * (second_reflection ? 0.5f : 1.0f))
                                     + (second_color * 0.5f);

            set_pixel(r.x, r.y, out_color);
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

