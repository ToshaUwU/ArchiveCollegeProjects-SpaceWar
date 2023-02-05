cmake_minimum_required(VERSION 3.11)

project(SpaceWar VERSION 1.0.0)
add_executable(spacewar ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

set_target_properties(spacewar PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO

	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/release"

	OUTPUT_NAME "sw"
)

if(MSVC)
	target_compile_options(spacewar PRIVATE /W4)
else()
	target_compile_options(spacewar PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(bgi STATIC IMPORTED)
set_target_properties(bgi PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bin/libbgi.a)
target_include_directories(spacewar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include)
target_link_libraries(spacewar bgi)