cmake_minimum_required(VERSION 3.16)

project(raytracer)

find_package(Threads REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(raytracer 
    src/Main.cpp
)
target_include_directories(raytracer PRIVATE
    include
)
target_link_libraries(raytracer PRIVATE
    Threads::Threads
    glm::glm
    SDL2::SDL2
)
set_target_properties(raytracer PROPERTIES
    CXX_STANDARD 17
)

