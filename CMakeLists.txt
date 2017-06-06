cmake_minimum_required(VERSION 2.6)
project(stst)
add_executable(stst WIN32
    main.c
    glad.c
    glad_wgl.c)
set_target_properties(stst PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${stst_SOURCE_DIR}")
include_directories(${stst_SOURCE_DIR}/include)
target_link_libraries(stst debug user32 gdi32 opengl32)
target_link_libraries(stst optimized user32 gdi32 opengl32)
