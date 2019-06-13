cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
project(bmfont_to_cpp_sdl2_example VERSION 1.0.0 LANGUAGES CXX)


set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(SDL2W 2.0.0 REQUIRED)


add_executable(${PROJECT_NAME} "")


set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		src/include
)

if (WIN32)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
			WIN32_EXECUTABLE TRUE
	)
endif ()

if (MINGW)
	target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
			-mconsole
	)
endif ()

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		SDL2W
)

if (MSVC)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
			LINK_FLAGS "/SUBSYSTEM:CONSOLE"
	)
endif ()

target_sources(
	${PROJECT_NAME}
	PRIVATE
		src/bmfont_to_cpp_sdl2_example.cpp
)

install(
	TARGETS
		${PROJECT_NAME}
	RUNTIME
		DESTINATION .
)
