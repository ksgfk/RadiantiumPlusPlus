message(STATUS "RAD build realtime module")
message(STATUS "RAD realtime find core module ${RAD_CORE_MODULE_NAME}")
message(STATUS "RAD realtime find glad module ${RAD_GLAD_MODULE_NAME}")

find_package(glfw3 CONFIG REQUIRED)

add_library(${RAD_REALTIME_MODULE_NAME} STATIC
    src/window.cpp
    src/opengl_context.cpp)
target_include_directories(${RAD_REALTIME_MODULE_NAME} PUBLIC
    "include")
target_link_libraries(${RAD_REALTIME_MODULE_NAME} PUBLIC glfw)
target_link_libraries(${RAD_REALTIME_MODULE_NAME} PUBLIC 
    ${RAD_CORE_MODULE_NAME}
    ${RAD_GLAD_MODULE_NAME})
