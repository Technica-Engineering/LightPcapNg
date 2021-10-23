cmake_minimum_required(VERSION 3.11)

# Try to find zstd in the cmake search path
find_package(zstd QUIET)
if(zstd_FOUND)
  # zstd was already found, so we don't resolve our dependency by ourself Check
  # if dependency was resolved by conan (find_package generator)
  if(CONAN_ZSTD_ROOT)
    set(light_zstd_dependency_name zstd::zstd)
  else()
    if(BUILD_SHARED_LIBS)
      set(light_zstd_dependency_name zstd::libzstd_shared)
    else()
      set(light_zstd_dependency_name zstd::libzstd_static)
    endif()
  endif()
else()
  # try finding the library on the system with the traditional method (no cmake
  # package on system)
  find_library(libzstd_static NAMES libzstd.a)
  find_library(libzstd_shared NAMES libzstd.so)
  if(libzstd_static AND libzstd_shared)
    if(BUILD_SHARED_LIBS)
      set(light_zstd_dependency_name ${libzstd_shared})
    else()
      set(light_zstd_dependency_name ${libzstd_static})
    endif()
  else()
    # compile library if not found
    include(FetchContent)

    FetchContent_Declare(
      zstd
      GIT_REPOSITORY https://github.com/facebook/zstd.git
      GIT_TAG v1.4.5)
    FetchContent_GetProperties(zstd POPULATED ZSTD_POPULATED)
    if(NOT ZSTD_POPULATED)
      FetchContent_Populate(zstd)
      add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    include_directories(${zstd_SOURCE_DIR}/lib)

    if(BUILD_SHARED_LIBS)
      set(light_zstd_dependency_name libzstd_shared)
    else()
      set(light_zstd_dependency_name libzstd_static)
    endif()
  endif()
endif()

add_definitions(-DUSE_ZSTD)
target_link_libraries(light_pcapng ${light_zstd_dependency_name})


# this pthread linking is required for the static linking of certain zstd
# version. See: https://github.com/facebook/zstd/pull/2097
if(NOT BUILD_SHARED_LIBS)
  find_package(Threads REQUIRED)
  target_link_libraries(light_pcapng ${CMAKE_THREAD_LIBS_INIT})
endif()
