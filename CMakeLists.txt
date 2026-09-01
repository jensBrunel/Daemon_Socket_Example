cmake_minimum_required(VERSION 3.5)
project(daemon_socket_example C CXX)

# Force gcc/g++ on Linux (optional)
if(NOT DEFINED CMAKE_C_COMPILER)
	set(CMAKE_C_COMPILER "gcc")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
	set(CMAKE_CXX_COMPILER "g++")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Discover and add third-party libraries located under either `third_party` or `src/third_party`
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party" OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party")
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
		set(_third_party_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
	else()
		set(_third_party_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party")
	endif()

	file(GLOB THIRD_PARTY_PATHS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${_third_party_dir}/*")
	list(SORT THIRD_PARTY_PATHS)

	# Ensure RapidJson (if present) is built first
	set(_rapidjson_rel "${_third_party_dir}/RapidJson")
	if(EXISTS "${_rapidjson_rel}")
		list(FIND THIRD_PARTY_PATHS "${_rapidjson_rel}" _found_idx)
		if(_found_idx EQUAL -1)
			list(INSERT THIRD_PARTY_PATHS 0 "${_rapidjson_rel}")
		endif()
	endif()

	foreach(tp_dir IN LISTS THIRD_PARTY_PATHS)
		if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${tp_dir}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${tp_dir}/CMakeLists.txt")
			add_subdirectory(${tp_dir})
		endif()
	endforeach()
endif()


# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

# Sources (C++ entrypoint)
add_executable(daemon_socket_example main.cpp Socket.cpp IniConfig.cpp)

# Install target
install(TARGETS daemon_socket_example DESTINATION bin)

# Convenience targets for building and cleaning with gcc on Linux
# Usage:
#   cmake -S . -B build
#   cmake --build build          # or: cmake --build build -- -j4
#   cmake --build build --target clean
add_custom_target(build
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Build project with gcc"
)

add_custom_target(cleanBuild
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Clean build artifacts"
)

# Custom target to configure+build using Ninja in an out-of-source directory 'build.ninja'
add_custom_target(ninja_build
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/build.ninja -G "Ninja"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/build.ninja --parallel
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Configure and build using Ninja (out-of-source: build.ninja)"
)

# Link RapidJSON headers (header-only INTERFACE target provided by RapidJSON)
target_link_libraries(daemon_socket_example PRIVATE RapidJSON)
