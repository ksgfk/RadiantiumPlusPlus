message(STATUS "RAD build editor")

add_executable(rad.offline.editor main.cpp editor_application.cpp)
target_link_libraries(rad.offline.editor rad.offline)
target_link_libraries(rad.offline.editor rad.realtime)
if(MSVC)
  target_compile_options(rad.offline.editor PUBLIC /utf-8 /fp:fast /arch:AVX2)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(rad.offline.editor PUBLIC /Ob2 /Oi /Ot /GF /Gy /GL) 
    target_link_options(rad.offline.editor PUBLIC /LTCG /OPT:REF,ICF)
  endif()
endif()
add_custom_command(TARGET rad.offline.editor POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fonts ${EXECUTABLE_OUTPUT_PATH}/fonts)
