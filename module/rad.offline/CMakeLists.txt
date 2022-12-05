message(STATUS "RAD build offline module")
message(STATUS "RAD offline find core module ${RAD_CORE_MODULE_NAME}")

option(RAD_FLOAT_32_WEIGHT  "RAD-Offline use float32 as Float?" ON)

find_package(TBB CONFIG REQUIRED)
find_package(embree 3 CONFIG REQUIRED)

add_library(${RAD_OFFLINE_MODULE_NAME} SHARED
    src/types.cpp
    src/frame.cpp
    src/transform.cpp
    src/warp.cpp
    src/distribution.cpp
    src/bounding_sphere.cpp
    src/fresnel.cpp
    src/microfacet.cpp
    src/interaction.cpp
    src/shape.cpp
    src/medium.cpp
    src/sampler.cpp
    src/volume.cpp
    src/scene.cpp
    src/build/build_context.cpp
    src/build/factory.cpp
    src/build/config_node_ext.cpp
    src/accel/embree.cpp
    src/shape/mesh.cpp
    src/shape/cube.cpp
    src/shape/sphere.cpp
    src/shape/rectangle.cpp
    src/shape/mesh_base.cpp
    src/light/area.cpp
    src/light/skybox.cpp
    src/light/point.cpp
    src/light/projection.cpp
    src/bsdf/diffuse.cpp
    src/bsdf/perfect_glass.cpp
    src/bsdf/perfect_mirror.cpp
    src/bsdf/two_side.cpp
    src/bsdf/disney.cpp
    src/bsdf/plastic.cpp
    src/bsdf/rough_metal.cpp
    src/bsdf/rough_plastic.cpp
    src/bsdf/rough_glass.cpp
    src/phase_function/isotropic.cpp
    src/sampler/independent.cpp
    src/texture/bitmap.cpp
    src/texture/chessboard.cpp
    src/volume/grid.cpp
    src/medium/homogeneous.cpp
    src/medium/heterogeneous.cpp
    src/camera/perspective.cpp
    src/renderer/sample_renderer.cpp
    src/renderer/path.cpp
    src/renderer/ao.cpp
    src/renderer/bdpt.cpp
    src/renderer/particle_tracer.cpp
    src/renderer/vol_path.cpp)
target_include_directories(${RAD_OFFLINE_MODULE_NAME} PUBLIC
    "include")
target_link_libraries(${RAD_OFFLINE_MODULE_NAME} PUBLIC
    ${RAD_CORE_MODULE_NAME})
target_link_libraries(${RAD_OFFLINE_MODULE_NAME} PRIVATE
    embree
    TBB::tbb)
if(MSVC)
  target_compile_options(${RAD_OFFLINE_MODULE_NAME} PUBLIC /utf-8 /fp:fast /arch:AVX2)
  # if(CMAKE_BUILD_TYPE STREQUAL "Release")
  #   target_compile_options(${RAD_OFFLINE_MODULE_NAME} PUBLIC /Ob2 /Oi /Ot /GF /Gy /GL) 
  #   target_link_options(${RAD_OFFLINE_MODULE_NAME} PUBLIC /LTCG /OPT:REF,ICF)
  # endif()
endif()
if(RAD_FLOAT_32_WEIGHT)
  target_compile_definitions(${RAD_OFFLINE_MODULE_NAME} PRIVATE RAD_USE_FLOAT32)
  message(STATUS "RAD offline use float32 as Float")
else()
  target_compile_definitions(${RAD_OFFLINE_MODULE_NAME} PRIVATE RAD_USE_FLOAT64)
  message(STATUS "RAD offline use float64 as Float")
endif()
set_target_properties(${RAD_OFFLINE_MODULE_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
