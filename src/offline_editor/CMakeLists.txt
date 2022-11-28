message(STATUS "RAD build editor")

if(WIN32)
find_package(directx-headers CONFIG REQUIRED)
endif()

set(RAD_EDITOR_SOURCE_FILES
    main.cpp
    window.cpp)
if(WIN32)
list(APPEND RAD_EDITOR_SOURCE_FILES
    dx12/render_context_dx12.cpp)
endif()

add_executable(rad.offline.editor ${RAD_EDITOR_SOURCE_FILES})
target_link_libraries(rad.offline.editor rad.offline)
if(WIN32)
target_link_libraries(rad.offline.editor d3dcompiler.lib
                                         d3d12.lib
                                         dxgi.lib)
target_link_libraries(rad.offline.editor Microsoft::DirectX-Guids Microsoft::DirectX-Headers)
endif()
