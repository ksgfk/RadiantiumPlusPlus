message(STATUS "RAD build offline.editor module")
message(STATUS "RAD offline.editor find realtime module ${RAD_REALTIME_MODULE_NAME}")

# 然后再搞编辑器...
add_executable(${RAD_OFFLINE_EDITOR_MODULE_NAME} 
    src/main.cpp
    src/ui_manager.cpp
    src/editor_application.cpp
    src/coms.cpp)
target_include_directories(${RAD_OFFLINE_EDITOR_MODULE_NAME} PUBLIC
    "include")
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
