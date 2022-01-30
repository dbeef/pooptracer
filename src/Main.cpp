#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "raytracer/Color.hpp"
#include "raytracer/Framebuffer.hpp"
#include "raytracer/scene/Plane.hpp"
#include "raytracer/scene/SceneEntity.hpp"
#include "raytracer/scene/Sphere.hpp"
#include "raytracer/scene/Triangle.hpp"

namespace
{
constexpr std::size_t THREAD_COUNT = 16;
constexpr std::size_t RAYS_TOTAL = Framebuffer::WIDTH * Framebuffer::HEIGHT;
constexpr std::size_t RAYS_PER_THREAD = std::ceil(static_cast<float>(Framebuffer::WIDTH * Framebuffer::HEIGHT) / THREAD_COUNT);
} // namespace

struct Camera
{
    explicit Camera(glm::vec3 pos)
        : position(pos){};

    Camera(){};

    static constexpr float f = 100;
    int w = Framebuffer::WIDTH;
    int h = Framebuffer::HEIGHT;
    glm::vec3 position;
};

std::atomic<bool> app_quit{false};
std::atomic<std::size_t> threads_finished_count{THREAD_COUNT};
Camera camera = Camera(glm::vec3(0.0f, 0.0f, -1.0f));
std::vector<std::shared_ptr<SceneEntity>> scene;
Framebuffer framebuffer;

struct RayThread
{
    std::mutex mtx;
    std::condition_variable cv;
    std::vector<Ray> rays;
    std::thread thread;

    void loop()
    {
        while (!app_quit)
        {
            std::unique_lock lck(mtx);
            // TODO: Spurious wakeup lambda check
            cv.wait(lck);

            for (const auto& r : rays)
            {
                Color color;
                Ray out;
                float intersection_distance = 999999;
                std::shared_ptr<SceneEntity> collided_entity = nullptr;
                for (const auto& scene_entity : scene)
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
                    framebuffer.set_pixel(r.x, r.y, Color::black());
                } else
                {
                    bool second_reflection = false;
                    Color second_color = {0, 0, 0};
                    for (const auto& scene_entity : scene)
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

                    framebuffer.set_pixel(r.x, r.y, out_color);
                }
            }

            threads_finished_count++;
        }
    }
};

std::array<RayThread, THREAD_COUNT> threads;

void init_raytracer()
{
    std::srand(0);

    std::vector<Ray> rays;

    // Generate rays:
    const auto half_height = camera.h / 2.0f;
    const auto half_width = camera.w / 2.0f;
    for (int y = 0; y < camera.h; y++)
    {
        for (int x = 0; x < camera.w; x++)
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

            r.position = camera.position;

            r.direction.x = x - half_width;
            r.direction.y = Camera::f;
            r.direction.z = y - half_height;

            r.direction = glm::normalize(r.direction);

            rays.push_back(r);
        }
    }
    // Fill scene:
    const glm::vec3 plane_pos = {0, 0, 0};
    const glm::vec3 plane_normal = {0, 0, 1};
    scene.push_back(std::make_shared<Plane>(plane_pos, plane_normal));
    scene.push_back(std::make_shared<Sphere>(glm::vec3{0, 2, -0.5f}, 1));
    scene.push_back(std::make_shared<Triangle>(
        glm::vec3{2, 2, -0.5f},
        glm::vec3{1, 0, 0}, glm::vec3{0, 1.0f, 0}, glm::vec3{0, 0.5f, -1}));
    scene.push_back(std::make_shared<Sphere>(glm::vec3{2, 5, -0.5f}, 1));
    scene.push_back(std::make_shared<Sphere>(glm::vec3{-2, 6, -0.5f}, 1));
    scene.push_back(std::make_shared<Sphere>(glm::vec3{-2, 2, -1.0f}, 0.3));
    scene.push_back(std::make_shared<Sphere>(glm::vec3{0.5, 0.5, -0.5f}, 0.20));

    std::size_t ray_index = 0;
    for (auto& ray_thread : threads)
    {
        for (std::size_t current = ray_index * RAYS_PER_THREAD,
                         end = (ray_index + 1) * RAYS_PER_THREAD;
             current < end;
             current++)
        {
            if (current >= RAYS_TOTAL)
            {
                break;
            }

            ray_thread.rays.push_back(rays.at(current));
        }
        std::cout << "This thread has: " << ray_thread.rays.size() << " rays  " << std::endl;
        ray_thread.thread = std::thread(&RayThread::loop, &ray_thread);
        ray_index++;
    }
}

void update_raytracer()
{
}

int main(int argc, char** argv)
{
    init_raytracer();

    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // create SDL window
    SDL_Window* window = SDL_CreateWindow("sdl2_pixelbuffer",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          Framebuffer::WIDTH * 4,
                                          Framebuffer::HEIGHT * 4,
                                          SDL_WINDOW_RESIZABLE);
    if (window == NULL)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return 1;
    }

    // create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, Framebuffer::WIDTH, Framebuffer::HEIGHT);

    // create texture
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        Framebuffer::WIDTH,
        Framebuffer::HEIGHT);
    if (texture == NULL)
    {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return 1;
    }

    // main loop
    bool should_quit = false;
    SDL_Event e;
    while (!should_quit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                should_quit = true;
            }
        }

        // update texture with new data
        int texture_pitch = 0;
        void* texture_pixels = NULL;
        if (SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch) != 0)
        {
            SDL_Log("Unable to lock texture: %s", SDL_GetError());
        } else
        {
            while (threads_finished_count != THREAD_COUNT)
            {
            }

            memcpy(texture_pixels, framebuffer.data(), texture_pitch * Framebuffer::HEIGHT);

            for (int i = 1; i < 2; i++)
            {
                auto& sphere = scene.at(i);
                const auto& position = sphere->get_position();
                glm::vec3 new_pos = position;
                static float timer = 0;
                timer += 0.05f;

                new_pos.y += std::sin(timer) / 20;
                new_pos.x += std::cos(timer) / 20;

                sphere->set_position(new_pos);

                sphere->recalc();
            }

            threads_finished_count = 0;

            for (auto& thread : threads)
            {
                thread.cv.notify_one();
            }
        }

        SDL_UnlockTexture(texture);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    app_quit = true;

    for (auto& ray_thread : threads)
    {
        ray_thread.cv.notify_one();
        ray_thread.thread.join();
    }

    return 0;
}
