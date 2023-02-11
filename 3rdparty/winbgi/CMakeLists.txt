cmake_minimum_required(VERSION 3.11)

project(winbgim)
add_library(winbgim STATIC)

set_target_properties(winbgim PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"

	PREFIX "lib"
	OUTPUT_NAME "winbgi"
)

if(MSVC)
	target_compile_options(winbgim PRIVATE /w)
else()
	target_compile_options(winbgim PRIVATE -w)
endif()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src lib_sources)
target_sources(winbgim PRIVATE ${lib_sources})

target_include_directories(winbgim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(winbgim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)