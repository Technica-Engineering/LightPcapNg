cmake_minimum_required(VERSION 3.11)

add_library(light_zstd INTERFACE)

# Try to find zstd in the cmake search path
find_package(zstd QUIET)
if(NOT zstd_FOUND)
  # zstd was not found, so we resolve our dependency by ourself

  # try finding the library on the system with the traditional method 
  # (no cmake package on system)
  find_path(ZSTD_INCLUDE_DIRS zstd.h)
  find_library(libzstd_static NAMES libzstd.a)
  find_library(libzstd_shared NAMES libzstd.so)

  if(NOT (ZSTD_INCLUDE_DIRS AND libzstd_static AND libzstd_shared))
    message("zstd: compiling")
    # compile library if not found
    include(FetchContent)

    FetchContent_Declare(
      zstd
      GIT_REPOSITORY https://github.com/facebook/zstd.git
      GIT_TAG v1.5.0
      SOURCE_SUBDIR "build/cmake")

    FetchContent_GetProperties(zstd POPULATED ZSTD_POPULATED)

    if(NOT ZSTD_POPULATED)
      FetchContent_Populate(zstd)
      add_subdirectory("${zstd_SOURCE_DIR}/build/cmake" "${zstd_BINARY_DIR}" EXCLUDE_FROM_ALL)
    endif()

    set(ZSTD_INCLUDE_DIRS "${zstd_SOURCE_DIR}/lib")
  endif()
endif()

# if dependency was resolved by conan (find_package generator)
if(TARGET zstd::zstd)
    set(LIBZSTD_TARGET "zstd::zstd") 
else()
    if(BUILD_SHARED_LIBS)
        set(LIBZSTD_TARGET "libzstd_shared") 
    else()
        set(LIBZSTD_TARGET "libzstd_static")
        # this pthread linking is required for the static linking of certain zstd
        # versions. See: https://github.com/facebook/zstd/pull/2097
        find_package(Threads REQUIRED)
        target_link_libraries(light_zstd INTERFACE ${CMAKE_THREAD_LIBS_INIT})
    endif()
    if(TARGET "zstd::${LIBZSTD_TARGET}")
        # find_package will export target with zstd:: prefix
        string(PREPEND LIBZSTD_TARGET, "zstd::")
    endif()
endif()

target_link_libraries(light_zstd INTERFACE "${LIBZSTD_TARGET}")
if(${ZSTD_INCLUDE_DIRS})
    target_include_directories(light_zstd INTERFACE "${ZSTD_INCLUDE_DIRS}")
endif()