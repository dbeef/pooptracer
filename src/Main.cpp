#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <fmt/format.h>

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
#include <exception>

#include "raytracer/Color.hpp"
#include "raytracer/Options.hpp"
#include "raytracer/Framebuffer.hpp"
#include "raytracer/scene/Plane.hpp"
#include "raytracer/scene/SceneEntity.hpp"
#include "raytracer/scene/Sphere.hpp"
#include "raytracer/scene/Triangle.hpp"

struct Camera
{
    explicit Camera(glm::vec3 pos)
        : position(pos){};

    Camera(){};

    static constexpr float f = 100;
    int w = Options::WIDTH;
    int h = Options::HEIGHT;
    glm::vec3 position;
};

std::atomic<bool> app_quit{false};
std::atomic<std::size_t> threads_finished_count{Options::THREAD_COUNT};
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

std::array<RayThread, Options::THREAD_COUNT> threads;

void init_raytracer()
{
    std::srand(Options::SEED);

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
        for (std::size_t current = ray_index * Options::RAYS_PER_THREAD,
                         end = (ray_index + 1) * Options::RAYS_PER_THREAD;
             current < end;
             current++)
        {
            if (current >= Options::RAYS_TOTAL)
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

int main(int, char**)
try
{
    init_raytracer();

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error(fmt::format("Unable to initialize SDL: %s", SDL_GetError()));
    }

    SDL_Window* window = SDL_CreateWindow("sdl2_pixelbuffer",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          Options::WIDTH * 4,
                                          Options::HEIGHT * 4,
                                          SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        throw std::runtime_error(fmt::format("Unable to create window: %s", SDL_GetError()));
    }

    SDL_Renderer* renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        throw std::runtime_error(fmt::format("Unable to create renderer: %s", SDL_GetError()));
    }

    SDL_RenderSetLogicalSize(renderer, Options::WIDTH, Options::HEIGHT);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        Options::WIDTH,
        Options::HEIGHT);

    if (!texture)
    {
        throw std::runtime_error(fmt::format("Unable to create texture: %s", SDL_GetError()));
    }

    bool should_quit = false;
    SDL_Event event;

    while (!should_quit)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                should_quit = true;
            }
        }

        // Rotate sphere;
        // TODO: Move this out of the loop
        auto& sphere = scene.at(1);
        const auto& position = sphere->get_position();
        glm::vec3 new_pos = position;
        static float timer = 0;
        timer += 0.05f;
        new_pos.y += std::sin(timer) / 20;
        new_pos.x += std::cos(timer) / 20;
        sphere->set_position(new_pos);
        sphere->recalc();

        int texture_pitch = 0;
        void* texture_pixels = nullptr;

        const auto lock_result = SDL_LockTexture(texture, nullptr, &texture_pixels, &texture_pitch);
        assert(lock_result == 0);

        while (threads_finished_count != Options::THREAD_COUNT)
        {
            // Active waiting while ray-threads have not finished writing to buffer
        }

        memcpy(texture_pixels, framebuffer.data(), texture_pitch * Options::HEIGHT);
        threads_finished_count = 0;

        for (auto& thread : threads)
        {
            thread.cv.notify_one();
        }

        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
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

    return EXIT_SUCCESS;
}
catch (const std::exception& e)
{
    std::cerr << "Unhandled exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (...)
{
    std::cerr << "Unknown exception" << std::endl;
    return EXIT_FAILURE;
}

