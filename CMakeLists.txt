cmake_minimum_required(VERSION 3.5)
project(daemon_socket_example C)

# Force gcc on Linux (optional)
if(NOT DEFINED CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER "gcc")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra")

# Sources
add_executable(daemon_socket_example main.c)

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
