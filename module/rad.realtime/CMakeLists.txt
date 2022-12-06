message(STATUS "RAD build realtime module")
message(STATUS "RAD realtime find core module ${RAD_CORE_MODULE_NAME}")
message(STATUS "RAD realtime find glad module ${RAD_GLAD_MODULE_NAME}")

find_package(glfw3 CONFIG REQUIRED)

# 首先单独给imgui弄个库
if(RAD_IS_BUILD_DEBUG)
  set(RAD_IMGUI_LIB_NAME "imgui_debug")
else()
  set(RAD_IMGUI_LIB_NAME "imgui")
endif()
add_library(${RAD_IMGUI_LIB_NAME} STATIC
    ${RAD_EXT_LIB_PATH}/imgui/imgui_demo.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui_draw.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui_tables.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui_widgets.cpp
    ${RAD_EXT_LIB_PATH}/imgui/imgui.cpp
    ${RAD_EXT_LIB_PATH}/imgui/backends/imgui_impl_opengl3.cpp # 我们使用ogl+glfw作为绘制后端
    ${RAD_EXT_LIB_PATH}/imgui/backends/imgui_impl_glfw.cpp)
target_include_directories(${RAD_IMGUI_LIB_NAME} PUBLIC
    ${RAD_EXT_LIB_PATH}/imgui)
target_link_libraries(${RAD_IMGUI_LIB_NAME} PRIVATE glfw)

# 然后再构建realtime
add_library(${RAD_REALTIME_MODULE_NAME} STATIC
    src/window.cpp
    src/opengl_context.cpp)
target_include_directories(${RAD_REALTIME_MODULE_NAME} PUBLIC
    "include")
target_link_libraries(${RAD_REALTIME_MODULE_NAME} PRIVATE glfw)
target_link_libraries(${RAD_REALTIME_MODULE_NAME} PUBLIC 
    ${RAD_CORE_MODULE_NAME}
    ${RAD_GLAD_MODULE_NAME}
    ${RAD_IMGUI_LIB_NAME})
