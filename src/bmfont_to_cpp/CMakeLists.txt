cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
project(bmfont_to_cpp VERSION 1.0.1 LANGUAGES CXX)


add_executable(${PROJECT_NAME} "")

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_sources(
	${PROJECT_NAME}
	PRIVATE
		src/bmfont_to_cpp.cpp
)

install(
	TARGETS
		${PROJECT_NAME}
	RUNTIME
		DESTINATION .
)
