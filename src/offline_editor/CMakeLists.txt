message(STATUS "RAD build editor")

add_executable(rad.offline.editor
    main.cpp
    window.cpp)
target_link_libraries(rad.offline.editor rad.offline)
