cmake_minimum_required(VERSION 3.16)

project(raytracer)

find_package(Threads REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(raytracer 
    src/Main.cpp
        include/raytracer/scene/SceneEntity.hpp include/raytracer/Color.hpp include/raytracer/Ray.hpp include/raytracer/scene/Plane.hpp include/raytracer/scene/Sphere.hpp include/raytracer/scene/Triangle.hpp include/raytracer/Framebuffer.hpp include/raytracer/Options.hpp)
target_include_directories(raytracer PRIVATE
    include
)
target_link_libraries(raytracer PRIVATE
    Threads::Threads
    glm::glm
    SDL2::SDL2
    fmt::fmt
)
set_target_properties(raytracer PROPERTIES
    CXX_STANDARD 17
)

