#
# 816CE emulator
# A 65816 emulator with instruction resolution complete with TUI debugger
#
# (C) Ray Clemens 2023
#

# This happens to be the version that I was using
# when the project was set up
cmake_minimum_required(VERSION 3.22)

# If no build type is specified, set the default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_COFIGURATION_TYPES)
  message("Building as 'MinSizeRel'.")
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Specify the build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug"
    "RelWithDebInfo"
    "Release"
    "MinSizeRel"
    )
endif()
    

project(816CE VERSION 0.1.0
              DESCRIPTION "65816 CPU emulation core and TUI debugger"
              LANGUAGES C)

set(exe_name 816ce)
            
set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -std=c99 -O0 -g")
set(LINK_FLAGS "-lncurses -lm")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS}")            
            
# Binary outputs
set(BINARY_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_CURRENT_BINARY_DIR ${BINARY_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR})
include_directories(${BINARY_OUTPUT_DIR})

# Project sources
include_directories("src")
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/cpu/*" "src/debugger/*" "src/hw/*")
file(GLOB_RECURSE SOURCES_UTIL RELATIVE ${CMAKE_SOURCE_DIR} "src/util/hashtable.*" "src/util/stack.*")

# Final executable
add_executable(${exe_name} ${SOURCES} ${SOURCES_UTIL})
target_link_libraries(${exe_name} ncurses m)

# Run
add_custom_target(run
  COMMAND "${exe_name}"
  DEPENDS "${exe_name}"
  USES_TERMINAL
  )

# Tests
set(test_dir test)
add_custom_target(test
  COMMAND "cd" "${CMAKE_SOURCE_DIR}/test" ";" "./run.sh"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${exe_name}" "${CMAKE_CURRENT_SOURCE_DIR}/test/runner.c" "${CMAKE_CURRENT_SOURCE_DIR}/test/json2cpu.py"
  )

# Cleanup (because it's nice)
add_custom_target(clean-build
  COMMAND rm -rf "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND rm -rf CMakeFiles
  COMMAND rm -rf Logs
  COMMAND rm -f CMakeCache.txt cmake_install.cmake
  COMMAND rm -f build.ninja
  COMMAND rm -f Makefile
  )
add_custom_target(clean-test
  COMMAND rm -rf "${CMAKE_SOURCE_DIR}/test/op_tests"
  COMMAND rm -rf "${CMAKE_SOURCE_DIR}/test/results"
  )
# add_custom_target(clean-full)
# add_dependencies(clean-full clean-build clean-test)


