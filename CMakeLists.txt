cmake_minimum_required(VERSION 3.11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()


project(light_pcapng C)

# Configuration

include_directories("include")

file(GLOB LIGHT_SOURCES src/*.c)

add_library(light_pcapng SHARED ${LIGHT_SOURCES})
add_library(light_pcapng_static STATIC ${LIGHT_SOURCES})

# ZSTD

option(LIGHT_USE_ZSTD "Compile with ZSTD support" ON)

if(LIGHT_USE_ZSTD)
    include(cmake/zstd.cmake)
    add_definitions(-DUSE_ZSTD)
    target_link_libraries(light_pcapng libzstd_shared)
    target_link_libraries(light_pcapng_static libzstd_static)
endif()

# Testing
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
