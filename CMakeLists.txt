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

file(GLOB LIGHT_SOURCES src/*.c)
set(light_pcapng_HEADERS
  include/light_io.h
  include/light_export.h
  include/light_io.h
  include/light_io_file.h
  include/light_io_mem.h
  include/light_io_zstd.h
  include/light_pcapng.h
  include/light_pcapng_ext.h
  include/light_special.h
  include/light_util.h
  )

add_library(light_pcapng ${LIGHT_SOURCES})
set_target_properties(light_pcapng PROPERTIES PUBLIC_HEADER
  "${light_pcapng_HEADERS}")

target_include_directories(light_pcapng PUBLIC "include")
# Export API symbols when the library is created

if (BUILD_SHARED_LIBS)
  target_compile_definitions(light_pcapng PRIVATE LIGHT_EXPORTS=1)
  target_compile_definitions(light_pcapng PUBLIC LIGHT_IMPORTS=1)
endif()

# ZSTD

option(LIGHT_USE_ZSTD "Compile with ZSTD support" ON)

if(LIGHT_USE_ZSTD)
    # Try to find zstd in the cmake search path
    find_package(zstd QUIET)
    if (zstd_FOUND)
        # zstd was already found, so we don't resolve our dependency by ourself
        # Check if dependency was resolved by conan (find_package generator)
        if (CONAN_ZSTD_ROOT)
          set(light_zstd_dependency_name zstd::zstd)
        else()
          # use the default cmake name
          set(light_zstd_dependency_name zstd)
        endif()
    else()
        include(cmake/zstd.cmake)
        if (BUILD_SHARED_LIBS)
          set(light_zstd_dependency_name libzstd_shared)
        else()
          set(light_zstd_dependency_name libzstd_static)
        endif()
    endif()
    add_definitions(-DUSE_ZSTD)
    target_link_libraries(light_pcapng ${light_zstd_dependency_name})
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Testing
    include(CTest)

    # We compile with warnings as error on non Windows,
    # We will make it even more strict in the future
    if(MSVC)
      target_compile_options(light_pcapng PRIVATE /W4)
    else()
      target_compile_options(light_pcapng PRIVATE -Werror)
    endif()
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS light_pcapng
  PUBLIC_HEADER DESTINATION include
  )
