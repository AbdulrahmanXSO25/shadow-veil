cmake_minimum_required(VERSION 3.24)
project(ShadowVeil VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(SDL3 REQUIRED CONFIG)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SV_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM SV_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

# All game code lives in a library so the test runner can drive the simulation headless.
add_library(sv_engine STATIC ${SV_SOURCES})
target_include_directories(sv_engine PUBLIC src)
target_compile_definitions(sv_engine PUBLIC
  GL_GLEXT_PROTOTYPES
  SV_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  SV_VERSION="${PROJECT_VERSION}")
target_compile_options(sv_engine PUBLIC -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
target_link_libraries(sv_engine PUBLIC SDL3::SDL3 OpenGL::OpenGL Freetype::Freetype ZLIB::ZLIB)

add_executable(shadow_veil src/main.cpp)
target_link_libraries(shadow_veil PRIVATE sv_engine)

add_executable(sv_score tools/trailer/score.cpp)
target_link_libraries(sv_score PRIVATE sv_engine)

add_executable(sv_tests tests/tests.cpp)
target_link_libraries(sv_tests PRIVATE sv_engine)

enable_testing()
add_test(NAME unit_and_simulation COMMAND sv_tests)

install(TARGETS shadow_veil RUNTIME DESTINATION bin)
install(DIRECTORY data assets DESTINATION share/shadow-veil)
