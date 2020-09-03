cmake_minimum_required(VERSION 3.11)

project(light_pcapng C)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    if(NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE Debug CACHE STRING
          "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
          FORCE)
    endif()

    # Dr Memory
    find_program(DRMEMORY_COMMAND "drmemory" DOC "Path to Dr. Memory binary")
    if(DRMEMORY_COMMAND)
        set(MEMORYCHECK_TYPE DrMemory)
        # Needed so Dr. Memory can work correctly
        # See https://dynamorio.org/drmemory_docs/page_prep.html
        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    endif()

endif()

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

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Testing
    include(CTest)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
