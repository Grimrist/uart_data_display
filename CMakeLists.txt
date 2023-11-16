cmake_minimum_required(VERSION 3.5)
 
project(imgui_test)

include(Y:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake)

## Find dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(freetype CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

## Create main executable
add_executable(main main.cpp)

target_link_libraries(
  main
  PRIVATE
    freetype
    glfw
    glad::glad
    glm::glm
    imgui::imgui
)


