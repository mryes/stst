cmake_minimum_required(VERSION 2.6)
project(stst)
add_executable(stst
    main.c)
target_link_libraries(stst debug user32 gdi32 opengl32)
target_link_libraries(stst optimized user32 gdi32 opengl32)
