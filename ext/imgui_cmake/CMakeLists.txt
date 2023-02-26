message(STATUS "RAD build imgui module")

find_package(glfw3 CONFIG REQUIRED)

add_library(${RAD_IMGUI_LIB_NAME} STATIC
    ${RAD_EXT_LIB_PATH}/imgui/imgui_demo.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui_draw.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui_tables.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui_widgets.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui.cpp
    ${RAD_EXT_LIB_PATH}/imgui/backends/imgui_impl_glfw.cpp
    ${RAD_EXT_LIB_PATH}/imgui/misc/cpp/imgui_stdlib.cpp)
target_include_directories(${RAD_IMGUI_LIB_NAME} PUBLIC
    ${RAD_EXT_LIB_PATH}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../imgui-filebrowser)
target_link_libraries(${RAD_IMGUI_LIB_NAME} PRIVATE glfw)
