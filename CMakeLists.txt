cmake_minimum_required(VERSION 3.0.0)
project(rad VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
  set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "" FORCE) # windows使用64位动态库
endif()

option(RAD_IS_BUILD_REALTIME  "RAD is build realtime?" ON)
option(RAD_IS_BUILD_OFFLINE_EDITOR  "RAD is build offline.editor" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(RAD_IS_BUILD_DEBUG TRUE)
  message(STATUS "RAD build debug")
else()
  set(RAD_IS_BUILD_DEBUG FALSE)
  message(STATUS "RAD build release")
endif()

if(RAD_IS_BUILD_DEBUG)
  set(RAD_CORE_MODULE_NAME "rad.core_debug")
  set(RAD_OFFLINE_MODULE_NAME "rad.offline_debug")
  set(RAD_OFFLINE_CLI_MODULE_NAME "rad.offline.cli_debug")
  set(RAD_OFFLINE_EDITOR_MODULE_NAME "rad.offline.editor_debug")
  set(RAD_REALTIME_MODULE_NAME "rad.realtime_debug")
  set(RAD_GLAD_MODULE_NAME "glad_debug")
else()
  set(RAD_CORE_MODULE_NAME "rad.core")
  set(RAD_OFFLINE_MODULE_NAME "rad.offline")
  set(RAD_OFFLINE_CLI_MODULE_NAME "rad.offline.cli")
  set(RAD_OFFLINE_EDITOR_MODULE_NAME "rad.offline.editor")
  set(RAD_REALTIME_MODULE_NAME "rad.realtime")
  set(RAD_GLAD_MODULE_NAME "glad")
endif()

set(RAD_EXT_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext")

add_subdirectory("module/rad.core") # Rad核心库
add_subdirectory("module/rad.offline") # 离线渲染库
add_subdirectory("module/rad.offline.cli") # 离线渲染控制台应用
if(RAD_IS_BUILD_REALTIME)
  add_subdirectory("${RAD_EXT_LIB_PATH}/glad") # 总之我不知道CMake为什么不是子文件夹就不能add, 傻逼cmake
  add_subdirectory("module/rad.realtime") # 可选构建实时渲染库
  if(RAD_IS_BUILD_OFFLINE_EDITOR)
    add_subdirectory("module/rad.offline.editor") # 可选构建离线渲染的编辑器
  endif()
endif()
