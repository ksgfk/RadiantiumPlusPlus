cmake_minimum_required(VERSION 3.0.0)
project(radiantium VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

option(RAD_FLOAT_32_WEIGHT  "use float32 as Float?" ON)

find_package(TBB CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(embree 3 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(OpenEXR CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h")

add_library(rad.offline
    src/offline/types.cpp
    src/offline/logger.cpp
    src/offline/color.cpp
    src/offline/transform.cpp
    src/offline/frame.cpp
    src/offline/warp.cpp
    src/offline/interaction.cpp
    src/offline/scene.cpp
    src/offline/distribution.cpp
    src/offline/shape.cpp
    src/offline/microfacet.cpp
    src/offline/fresnel.cpp
    src/offline/memory.cpp
    src/offline/utility/image.cpp
    src/offline/utility/stop_watch.cpp
    src/offline/asset/triangle_model.cpp
    src/offline/asset/wavefront_obj_reader.cpp
    src/offline/asset/model_obj.cpp
    src/offline/asset/image_default.cpp
    src/offline/build/config_node.cpp
    src/offline/build/build_context.cpp
    src/offline/shape/mesh.cpp
    src/offline/shape/sphere.cpp
    src/offline/camera/perspective.cpp
    src/offline/accel/embree.cpp
    src/offline/sampler/independent.cpp
    src/offline/renderer/sample_renderer.cpp
    src/offline/renderer/ao.cpp
    src/offline/renderer/direct.cpp
    src/offline/renderer/gbuffer.cpp
    src/offline/renderer/path.cpp
    src/offline/bsdf/diffuse.cpp
    src/offline/bsdf/rough_metal.cpp
    src/offline/bsdf/perfect_mirror.cpp
    src/offline/bsdf/perfect_glass.cpp
    src/offline/light/area.cpp
    src/offline/texture/bitmap.cpp)

if(RAD_FLOAT_32_WEIGHT)
  target_compile_definitions(rad.offline PRIVATE RAD_USE_FLOAT32)
  message(STATUS "RAD use float32")
else()
  target_compile_definitions(rad.offline PRIVATE RAD_USE_FLOAT64)
  message(STATUS "RAD use float64")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "RAD in Release mode")
else()
  target_compile_definitions(rad.offline PRIVATE RAD_DEFINE_DEBUG)
  message(STATUS "RAD in Debug mode")
endif()
if(MSVC)
target_compile_options(rad.offline PUBLIC /utf-8)
endif()
target_include_directories(rad.offline PUBLIC include)
target_include_directories(rad.offline PRIVATE ${STB_INCLUDE_DIRS})
target_link_libraries(rad.offline PRIVATE TBB::tbb)
target_link_libraries(rad.offline PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(rad.offline PUBLIC spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(rad.offline PRIVATE embree)
target_link_libraries(rad.offline PRIVATE Eigen3::Eigen)
target_link_libraries(rad.offline PRIVATE OpenEXR::IlmImf OpenEXR::IlmImfUtil OpenEXR::IlmImfConfig)

add_executable(rad.offline.cli
    src/offline_cli/main.cpp)
target_link_libraries(rad.offline.cli rad.offline)
