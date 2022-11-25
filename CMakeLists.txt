cmake_minimum_required(VERSION 3.0.0)
project(radiantium VERSION 0.1.0)

if(WIN32)
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "" FORCE) # win下目前只能用动态库, vcpkg里无法为openvdb构建静态库...
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

option(RAD_FLOAT_32_WEIGHT  "use float32 as Float?" ON)
option(RAD_BUILD_EDITOR "if build RAD editor?" ON)
set(RAD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(TBB CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(embree 3 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(OpenEXR CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h")
find_package(OpenVDB CONFIG REQUIRED)

if(RAD_BUILD_EDITOR)
  add_subdirectory(src/offline_editor)
endif()

add_library(rad.offline SHARED
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
    src/offline/medium.cpp
    src/offline/bounding_sphere.cpp
    src/offline/api.cpp
    src/offline/utility/image.cpp
    src/offline/utility/stop_watch.cpp
    src/offline/asset/triangle_model.cpp
    src/offline/asset/wavefront_obj_reader.cpp
    src/offline/asset/model_obj.cpp
    src/offline/asset/image_default.cpp
    src/offline/asset/image_exr.cpp
    src/offline/asset/static_grid.cpp
    src/offline/asset/volume_vdb.cpp
    src/offline/asset/volume_mitsuba_vol.cpp
    src/offline/build/config_node.cpp
    src/offline/build/build_context.cpp
    src/offline/shape/mesh.cpp
    src/offline/shape/sphere.cpp
    src/offline/shape/rectangle.cpp
    src/offline/shape/cube.cpp
    src/offline/camera/perspective.cpp
    src/offline/accel/embree.cpp
    src/offline/sampler/independent.cpp
    src/offline/renderer/sample_renderer.cpp
    src/offline/renderer/ao.cpp
    src/offline/renderer/direct.cpp
    src/offline/renderer/gbuffer.cpp
    src/offline/renderer/path.cpp
    src/offline/renderer/vol_path.cpp
    src/offline/renderer/particle_tracer.cpp
    src/offline/renderer/bdpt.cpp
    src/offline/bsdf/diffuse.cpp
    src/offline/bsdf/rough_metal.cpp
    src/offline/bsdf/perfect_mirror.cpp
    src/offline/bsdf/perfect_glass.cpp
    src/offline/bsdf/two_side.cpp
    src/offline/bsdf/rough_glass.cpp
    src/offline/bsdf/disney.cpp
    src/offline/bsdf/plastic.cpp
    src/offline/bsdf/rough_plastic.cpp
    src/offline/light/area.cpp
    src/offline/light/skybox.cpp
    src/offline/light/point.cpp
    src/offline/light/projection.cpp
    src/offline/texture/bitmap.cpp
    src/offline/texture/chessboard.cpp
    src/offline/medium/homogeneous.cpp
    src/offline/medium/heterogeneous.cpp
    src/offline/phase_function/isotropic.cpp
    src/offline/volume/volume.cpp
    src/offline/volume/grid.cpp)
target_compile_definitions(rad.offline PRIVATE RAD_BUILD_SHARED)
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
  target_compile_options(rad.offline PUBLIC /utf-8 /fp:fast /arch:AVX2)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(rad.offline PUBLIC /Ob2 /Oi /Ot /GF /Gy /GL)
  endif()
endif()
target_include_directories(rad.offline PUBLIC ${RAD_INCLUDE_DIR})
target_include_directories(rad.offline PUBLIC ${STB_INCLUDE_DIRS})
target_link_libraries(rad.offline PUBLIC TBB::tbb)
target_link_libraries(rad.offline PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(rad.offline PUBLIC spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(rad.offline PUBLIC embree)
target_link_libraries(rad.offline PUBLIC Eigen3::Eigen)
target_link_libraries(rad.offline PUBLIC OpenEXR::OpenEXR)
target_link_libraries(rad.offline PUBLIC OpenVDB::openvdb)
set_target_properties(rad.offline PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
target_link_options(rad.offline PUBLIC /LTCG /OPT:REF,ICF)

add_executable(rad.offline.cli
    src/offline_cli/main.cpp)
target_link_libraries(rad.offline.cli rad.offline)
target_link_options(rad.offline.cli PUBLIC /LTCG /OPT:REF,ICF)

# emm, 要编译两次, 第一次生成pgd文件, 然后再配置一次cmake, 接着再build一次才能真正启动PGO
# 而且速度也没啥变化...毕竟耗时大头是求交
# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#   set(PGO_FILE_NAME "rad.offline.cli.pgd")
#   set(PGO_FILE_PATH "${EXECUTABLE_OUTPUT_PATH}/${PGO_FILE_NAME}")
#   message(STATUS "RAD pgo file path: ${PGO_FILE_PATH}")
#   if(EXISTS "${PGO_FILE_PATH}")
#     message(STATUS "RAD find pgo file")
#     target_link_options(rad.offline PUBLIC /LTCG /USEPROFILE /OPT:REF,ICF)
#   else()
#     message(STATUS "RAD cannot find pgo file")
#     target_link_options(rad.offline PUBLIC /LTCG /GENPROFILE /OPT:REF,ICF)
#   endif()
# endif()
