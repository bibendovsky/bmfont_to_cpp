cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
project(bmfont_to_cpp_solution VERSION 1.0.0)

option(BMF2CPP_BUILD_SDL2_EXAMPLE "Build SDL2 example program." ON)

add_subdirectory(src/bmfont_to_cpp)

if (BMF2CPP_BUILD_SDL2_EXAMPLE)
	add_subdirectory(src/bmfont_to_cpp_sdl2_example)
endif ()
