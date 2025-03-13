cmake_minimum_required(VERSION 3.30)
project(Lab2)

set(CMAKE_CXX_STANDARD 20)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLFW and GLEW
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(FREETYPE REQUIRED freetype2)

set(FREEGLUT_INCLUDE_DIR "/opt/homebrew/include")
set(FREEGLUT_LIBRARY "/opt/homebrew/lib/libglut.dylib")

include_directories(
        ${GLEW_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${FREEGLUT_INCLUDE_DIR}
        ${FREETYPE_INCLUDE_DIRS}
)

link_directories(
        ${GLEW_LIBRARY_DIRS}
        ${GLFW_LIBRARY_DIRS}
        ${FREETYPE_LIBRARY_DIRS}
)

add_executable(Lab2 main.cpp)
target_link_libraries(
        Lab2
        ${GLEW_LIBRARIES}
        ${GLFW_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
        ${FREEGLUT_LIBRARY}
        ${FREETYPE_LIBRARIES}
)
