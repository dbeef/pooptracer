#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "pooptracer/Framebuffer.hpp"
#include "pooptracer/Options.hpp"
#include "pooptracer/Ray.hpp"
#include "pooptracer/scene/SceneEntity.hpp"

class RayThread
{
public:
    void add_ray(const Ray& ray)
    {
        _rays.push_back(ray);
    }

    void notify()
    {
        _cv.notify_one();
    }

    void join()
    {
        notify();
        _thread.join();
    }

    void start(std::atomic_bool& app_quit,
               std::atomic<std::size_t>& threads_finished_counter,
               Framebuffer& framebuffer,
               std::vector<std::shared_ptr<SceneEntity>>& scene)
    {
        assert(!_thread.joinable());
        _thread = std::thread(&RayThread::loop, this, std::ref(app_quit), std::ref(threads_finished_counter), std::ref(framebuffer), std::ref(scene));
    }

    void loop(std::atomic_bool& app_quit,
              std::atomic<std::size_t>& threads_finished_counter,
              Framebuffer& framebuffer,
              std::vector<std::shared_ptr<SceneEntity>>& scene)
    {
        while (!app_quit)
        {
            std::unique_lock lck(_mtx);
            // TODO: Spurious wakeup lambda check
            _cv.wait(lck);

            for (const auto& r : _rays)
            {
                Color color;
                Ray out;
                float intersection_distance = std::numeric_limits<float>::max();
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
                    framebuffer.set_pixel(r.x, r.y, Options::BACKGROUND);
                }
                else
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

            threads_finished_counter++;
        }
    }

private:
    std::mutex _mtx;
    std::condition_variable _cv;
    std::vector<Ray> _rays;
    std::thread _thread;
};
