cmake_minimum_required(VERSION 3.11)

add_library(light_zstd INTERFACE)

function(zstd_libraries SHARED_LIB STATIC_LIB)
    if(BUILD_SHARED_LIBS)
        target_link_libraries(light_zstd INTERFACE ${SHARED_LIB})
    else()
        target_link_libraries(light_zstd INTERFACE ${STATIC_LIB})
        # this pthread linking is required for the static linking of certain zstd
        # versions. See: https://github.com/facebook/zstd/pull/2097
        find_package(Threads REQUIRED)
        target_link_libraries(light_zstd INTERFACE ${CMAKE_THREAD_LIBS_INIT})
    endif()
endfunction()

# Try to find zstd in the cmake search path
find_package(zstd QUIET)
if(zstd_FOUND)
    if(TARGET zstd::zstd)
        # dependency was resolved by conan (find_package generator)
        target_link_libraries(light_zstd INTERFACE zstd::zstd)
    else()
        zstd_libraries(zstd::libzstd_shared zstd::libzstd_static)
    endif()
else()
    # zstd was not found, so we do not resolve our dependency by ourself
    # try finding the library on the system with the traditional method
    # (no cmake package on system)
    find_path(ZSTD_INCLUDE_DIRS zstd.h)
    find_library(ZSTD_STATIC_LIB NAMES libzstd.a)
    find_library(ZSTD_SHARED_LIB NAMES libzstd.so)

    if(ZSTD_INCLUDE_DIRS AND ZSTD_STATIC_LIB AND ZSTD_SHARED_LIB)
        zstd_libraries("${ZSTD_SHARED_LIB}" "${ZSTD_STATIC_LIB}")
        target_include_directories(light_zstd INTERFACE "${ZSTD_INCLUDE_DIRS}")
        set(zstd_FOUND ON)
    endif()
endif()

if(NOT zstd_FOUND)
    # compile library if not found
    include(FetchContent)

    FetchContent_Declare(
        zstd
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        # We need a version with top level CMakeLists.txt file
        # No tag have it yet, so use this commit
        GIT_TAG 448cd34
    )
    FetchContent_MakeAvailable(zstd)

    zstd_libraries(libzstd_shared libzstd_static)
endif()
