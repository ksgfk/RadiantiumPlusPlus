message(STATUS "RAD build realtime module")

if(WIN32)
find_package(directx-headers CONFIG REQUIRED)
endif()

set(RAD_EDITOR_SOURCE_FILES
    window.cpp)
if(WIN32)
list(APPEND RAD_EDITOR_SOURCE_FILES
    dx12/device.cpp
    dx12/upload_buffer.cpp
    dx12/readback_buffer.cpp
    dx12/default_buffer.cpp
    dx12/texture.cpp
    dx12/frame_resource.cpp
    dx12/resource_state.cpp
    dx12/render_context_dx12.cpp)
endif()

add_library(rad.realtime STATIC ${RAD_EDITOR_SOURCE_FILES})
target_link_libraries(rad.realtime PUBLIC rad.core)
if(WIN32)
target_link_libraries(rad.realtime PUBLIC 
    d3dcompiler.lib
    d3d12.lib
    dxgi.lib)
target_link_libraries(rad.realtime PUBLIC Microsoft::DirectX-Guids Microsoft::DirectX-Headers)
endif()
