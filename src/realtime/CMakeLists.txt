message(STATUS "RAD build realtime module")

option(RAD_REALTIME_USE_D3D12 "if RAD-Realtime use D3D12?" ON)
option(RAD_REALTIME_USE_OPENGL "if RAD-Realtime use OpenGL?" ON)

if(NOT WIN32)
set(RAD_REALTIME_USE_D3D12 FALSE)
message(STATUS "RAD-Realtime only build D3D12 on Windows")
endif()

if(RAD_REALTIME_USE_D3D12)
find_package(directx-headers CONFIG REQUIRED)
message(STATUS "RAD-Realtime build D3D12")
endif()
if(RAD_REALTIME_USE_OPENGL)
find_package(glfw3 CONFIG REQUIRED)
message(STATUS "RAD-Realtime build OpenGL")
endif()

set(RAD_REALTIME_SOURCE_FILES
    window.cpp)
if(RAD_REALTIME_USE_D3D12)
list(APPEND RAD_REALTIME_SOURCE_FILES
    dx12/device.cpp
    dx12/upload_buffer.cpp
    dx12/readback_buffer.cpp
    dx12/default_buffer.cpp
    dx12/texture.cpp
    dx12/frame_resource.cpp
    dx12/resource_state.cpp
    dx12/render_context_dx12.cpp)
endif()
if(RAD_REALTIME_USE_OPENGL)
list(APPEND RAD_REALTIME_SOURCE_FILES
    "../../ext/glad/src/gl.c"
    opengl/utility.cpp
    opengl/render_context_ogl.cpp)
endif()

add_library(rad.realtime STATIC ${RAD_REALTIME_SOURCE_FILES})
target_link_libraries(rad.realtime PUBLIC rad.core)
if(RAD_REALTIME_USE_D3D12)
target_compile_definitions(rad.realtime PRIVATE RAD_REALTIME_BUILD_D3D12)
target_link_libraries(rad.realtime PUBLIC 
    d3dcompiler.lib
    d3d12.lib
    dxgi.lib)
target_link_libraries(rad.realtime PUBLIC Microsoft::DirectX-Guids Microsoft::DirectX-Headers)
endif()
if(RAD_REALTIME_USE_OPENGL)
target_compile_definitions(rad.realtime PRIVATE RAD_REALTIME_BUILD_OPENGL)
target_link_libraries(rad.realtime PRIVATE glfw)
target_include_directories(rad.realtime PUBLIC "../../ext/glad/include")
endif()
