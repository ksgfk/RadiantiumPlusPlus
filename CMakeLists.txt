cmake_minimum_required(VERSION 3.0.0)
project(radiantium VERSION 0.1.0)

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

add_library(radiantium.core
    src/core/radiantium.cpp
    src/core/ray.cpp
    src/core/spectrum.cpp
    src/core/image_utility.cpp
    src/core/model.cpp
    src/core/logger.cpp
    src/core/transform.cpp
    src/core/frame.cpp
    src/core/math_ext.cpp
    src/core/interaction.cpp
    src/core/build_context.cpp
    src/core/factory.cpp
    src/core/config_node.cpp
    src/core/wavefront_obj_reader.cpp
    src/core/accel/embree_accel.cpp
    src/core/shape/mesh.cpp
    src/core/camera/perspective.cpp)

if(RAD_FLOAT_32_WEIGHT)
  target_compile_definitions(radiantium.core PRIVATE RAD_USE_FLOAT32)
  message(STATUS "RAD use float32")
else()
  target_compile_definitions(radiantium.core PRIVATE RAD_USE_FLOAT64)
  message(STATUS "RAD use float64")
endif()
target_include_directories(radiantium.core PUBLIC include)
target_link_libraries(radiantium.core PRIVATE TBB::tbb)
target_link_libraries(radiantium.core PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(radiantium.core PUBLIC spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(radiantium.core PRIVATE embree)
target_link_libraries(radiantium.core PRIVATE Eigen3::Eigen)
target_link_libraries(radiantium.core PRIVATE OpenEXR::IlmImf OpenEXR::IlmImfUtil OpenEXR::IlmImfConfig)

add_executable(radiantium.cli
    src/cli/main.cpp)
target_link_libraries(radiantium.cli radiantium.core)
