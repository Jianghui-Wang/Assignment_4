cmake_minimum_required(VERSION 3.21)
project(assignment LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    Source/main.cpp
)

# macOS deprecated OpenGL in 10.14; silence the wall of warnings.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GL_SILENCE_DEPRECATION
)

if(APPLE)
    # Prefer Homebrew-installed GLEW (the legacy macOS one in OpenGL.framework
    # is way too old). Path is the standard Apple-Silicon brew prefix.
    set(HOMEBREW_PREFIX "/opt/homebrew" CACHE PATH "Homebrew prefix")

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${HOMEBREW_PREFIX}/include
    )
    target_link_directories(${PROJECT_NAME} PRIVATE
        ${HOMEBREW_PREFIX}/lib
    )

    find_library(OPENGL_LIBRARY  OpenGL)   # provides GL and GLU
    find_library(GLUT_LIBRARY    GLUT)     # built-in deprecated framework
    find_library(COCOA_LIBRARY   Cocoa)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        GLEW                               # from brew
        ${OPENGL_LIBRARY}
        ${GLUT_LIBRARY}
        ${COCOA_LIBRARY}
    )
else()
    find_package(OpenGL REQUIRED)
    find_package(GLUT   REQUIRED)
    find_package(GLEW   REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL OpenGL::GLU GLUT::GLUT GLEW::GLEW
    )
endif()
