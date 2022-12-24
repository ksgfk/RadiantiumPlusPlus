message(STATUS "RAD build core module")

find_package(Eigen3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h")
find_package(OpenEXR CONFIG REQUIRED)
find_package(OpenVDB CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(${RAD_CORE_MODULE_NAME} SHARED
    src/logger.cpp
    src/triangle_model.cpp
    src/wavefront_obj_reader.cpp
    src/image_reader.cpp
    src/volume_grid.cpp
    src/volume_reader.cpp
    src/location_resolver.cpp
    src/memory.cpp
    src/factory.cpp
    src/common.cpp
    src/asset.cpp
    src/asset/image_stb.cpp
    src/asset/image_exr.cpp
    src/asset/image_hdr.cpp
    src/asset/model_obj.cpp
    src/asset/volume_mitsuba_vol.cpp
    src/asset/volume_vdb.cpp)
target_include_directories(${RAD_CORE_MODULE_NAME} PUBLIC
    "include")
target_include_directories(${RAD_CORE_MODULE_NAME} PRIVATE #我们不希望外部去调用任何stb的函数
    ${STB_INCLUDE_DIRS})
target_link_libraries(${RAD_CORE_MODULE_NAME} PUBLIC
    Eigen3::Eigen
    spdlog::spdlog spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json)
target_link_libraries(${RAD_CORE_MODULE_NAME} PRIVATE
    OpenEXR::OpenEXR #基本上只用到.exr格式解析的功能, 不需要开放API给其他模块
    OpenVDB::openvdb) #OpenVDB也是，只解析
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(${RAD_CORE_MODULE_NAME} PUBLIC RAD_DEFINE_DEBUG)
endif()
target_compile_definitions(${RAD_CORE_MODULE_NAME} PUBLIC RAD_BUILD_SHARED)
if(MSVC)
  target_compile_options(${RAD_CORE_MODULE_NAME} PUBLIC /utf-8)
endif()
set_target_properties(${RAD_CORE_MODULE_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
