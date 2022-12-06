message(STATUS "RAD build offline.editor module")
message(STATUS "RAD offline.editor find realtime module ${RAD_REALTIME_MODULE_NAME}")

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
    ${RAD_EXT_LIB_PATH}/imgui/backends/imgui_impl_glfw.cpp
    # 从这里开始是ImGuizmo，方便起见就放一起了...
    ${RAD_EXT_LIB_PATH}/ImGuizmo/GraphEditor.cpp
    ${RAD_EXT_LIB_PATH}/ImGuizmo/ImCurveEdit.cpp
    ${RAD_EXT_LIB_PATH}/ImGuizmo/ImGradient.cpp
    ${RAD_EXT_LIB_PATH}/ImGuizmo/ImGuizmo.cpp
    ${RAD_EXT_LIB_PATH}/ImGuizmo/ImSequencer.cpp)
target_include_directories(${RAD_IMGUI_LIB_NAME} PUBLIC
    ${RAD_EXT_LIB_PATH}/imgui
    ${RAD_EXT_LIB_PATH}/ImGuizmo)
target_link_libraries(${RAD_IMGUI_LIB_NAME} PRIVATE glfw)

# 然后再搞编辑器...
add_executable(${RAD_OFFLINE_EDITOR_MODULE_NAME} 
    main.cpp)
target_link_libraries(${RAD_OFFLINE_EDITOR_MODULE_NAME} 
    ${RAD_REALTIME_MODULE_NAME}
    ${RAD_IMGUI_LIB_NAME})
set_target_properties(${RAD_OFFLINE_EDITOR_MODULE_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
add_custom_command(TARGET ${RAD_OFFLINE_EDITOR_MODULE_NAME} POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fonts ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/fonts)
