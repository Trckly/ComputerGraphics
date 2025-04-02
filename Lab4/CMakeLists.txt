cmake_minimum_required(VERSION 3.30)
project(Lab4)

set(CMAKE_CXX_STANDARD 20)

# Find OpenGL
find_package(OpenGL REQUIRED)

# macOS-specific configuration
if(APPLE)
    # Find GLFW, GLEW, and FreeType using pkg-config
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

    add_executable(Lab4 main.cpp)
    target_link_libraries(
            Lab4
            ${GLEW_LIBRARIES}
            ${GLFW_LIBRARIES}
            ${OPENGL_gl_LIBRARY}
            ${FREEGLUT_LIBRARY}
            ${FREETYPE_LIBRARIES}
    )
endif()

# Windows-specific configuration
if(WIN32)
    # Set include directories
    set(GLEW_INCLUDE_DIR "C:/OpenGLLibraries/glew/include")
    set(GLFW_INCLUDE_DIR "C:/OpenGLLibraries/glfw/include")
    set(FREETYPE_INCLUDE_DIR "C:/OpenGLLibraries/freetype/include")
    set(GLM_INCLUDE_DIR "C:/OpenGLLibraries/glm")  # Adjust this path as needed

    # Set library paths
    set(GLEW_LIBRARY "C:/OpenGLLibraries/glew/lib/Debug/x64/glew32d.lib")
    set(GLFW_LIBRARY "C:/OpenGLLibraries/glfw/build/src/Debug/glfw3.lib")
    set(FREETYPE_LIBRARY "C:/OpenGLLibraries/freetype/objs/x64/Debug/freetype.lib")
    set(OPENGL_LIBRARY "opengl32.lib")  # Standard OpenGL library on Windows

    include_directories(
            ${GLEW_INCLUDE_DIR}
            ${GLFW_INCLUDE_DIR}
            ${FREETYPE_INCLUDE_DIR}
            ${GLM_INCLUDE_DIR}
    )

    # Add link directories if needed
#    link_directories(
#            "C:/OpenGLLibraries/glew/lib/Release/x64"
#            "C:/OpenGLLibraries/glfw/build/src/Debug"
#            "C:/OpenGLLibraries/freetype/objs"
#    )

    add_executable(Lab3 main.cpp)
    target_link_libraries(
            Lab4
            ${GLEW_LIBRARY}
            ${GLFW_LIBRARY}
            ${FREETYPE_LIBRARY}
            ${OPENGL_LIBRARY}
    )
endif()