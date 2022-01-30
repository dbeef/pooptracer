#include <SDL.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "pooptracer/Color.hpp"
#include "pooptracer/Framebuffer.hpp"
#include "pooptracer/Options.hpp"
#include "pooptracer/RayThread.hpp"
#include "pooptracer/scene/Plane.hpp"
#include "pooptracer/scene/SceneEntity.hpp"
#include "pooptracer/scene/Sphere.hpp"
#include "pooptracer/scene/Triangle.hpp"

namespace
{
std::atomic<bool> app_quit{false};
std::atomic<std::size_t> threads_finished_counter{Options::THREAD_COUNT};
Framebuffer framebuffer{};
std::vector<std::shared_ptr<SceneEntity>> scene;
std::array<RayThread, Options::THREAD_COUNT> threads;
} // namespace

void init_raytracer()
{
    std::srand(Options::SEED);

    std::vector<Ray> rays;

    // Generate rays:
    const auto half_height = Options::HEIGHT / 2.0f;
    const auto half_width = Options::WIDTH / 2.0f;
    for (int y = 0; y < Options::HEIGHT; y++)
    {
        for (int x = 0; x < Options::WIDTH; x++)
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

            r.position = Options::CAMERA_POSITION;

            r.direction.x = x - half_width;
            r.direction.y = Options::CAMERA_F;
            r.direction.z = y - half_height;

            r.direction = glm::normalize(r.direction);

            rays.push_back(r);
        }
    }

    // Fill scene:
    // TODO: Should be defined outside code, i.e in YML
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
    for (auto& thread : threads)
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

            thread.add_ray(rays.at(current));
        }

        thread.start(app_quit, threads_finished_counter, framebuffer, scene);
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

    SDL_Window* window = SDL_CreateWindow("pooptracer",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          Options::WIDTH * Options::UPSCALE_MODIFIER,
                                          Options::HEIGHT * Options::UPSCALE_MODIFIER,
                                          SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        throw std::runtime_error(fmt::format("Unable to create window: %s", SDL_GetError()));
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

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

        while (threads_finished_counter != Options::THREAD_COUNT)
        {
            // Active waiting while ray-threads have not finished writing to buffer
        }

        memcpy(texture_pixels, framebuffer.data(), texture_pitch * Options::HEIGHT);
        threads_finished_counter = 0;

        for (auto& thread : threads)
        {
            thread.notify();
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
        ray_thread.join();
    }

    return EXIT_SUCCESS;
} catch (const std::exception& e)
{
    std::cerr << "Unhandled exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
} catch (...)
{
    std::cerr << "Unknown exception" << std::endl;
    return EXIT_FAILURE;
}
