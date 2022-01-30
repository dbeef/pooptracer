cmake_minimum_required(VERSION 3.16)

project(raytracer)

find_package(Threads REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(raytracer 
    src/Main.cpp
    include/pooptracer/scene/SceneEntity.hpp
    include/pooptracer/Color.hpp
    include/pooptracer/Ray.hpp
    include/pooptracer/scene/Plane.hpp
    include/pooptracer/scene/Sphere.hpp
    include/pooptracer/scene/Triangle.hpp
    include/pooptracer/Framebuffer.hpp
    include/pooptracer/Options.hpp
    include/pooptracer/RayThread.hpp
)
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

