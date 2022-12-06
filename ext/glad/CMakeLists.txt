message(STATUS "RAD build glad module")

add_library(${RAD_GLAD_MODULE_NAME} STATIC
    src/gl.c)
target_include_directories(${RAD_GLAD_MODULE_NAME} PUBLIC
    "include")
