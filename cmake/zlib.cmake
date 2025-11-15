cmake_minimum_required(VERSION 3.11)

add_library(light_zlib INTERFACE)

# dependency was resolved by conan
if(TARGET CONAN_PKG::zlib-ng)
    target_link_libraries(light_zlib INTERFACE CONAN_PKG::zlib-ng)
    return()
endif()
if(TARGET CONAN_PKG::zlib)
    target_link_libraries(light_zlib INTERFACE CONAN_PKG::zlib)
    return()
endif()

# Try to find zlib in the cmake search path
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    target_link_libraries(light_zlib INTERFACE ZLIB::ZLIB)
    return()
endif()

# compile library if not found
include(FetchContent)

FetchContent_Declare(
    zlib
    GIT_REPOSITORY "https://github.com/madler/zlib.git"
    GIT_TAG "v1.3.1"
)
FetchContent_MakeAvailable(zlib)

target_include_directories(zlib PUBLIC ${zlib_BINARY_DIR} ${zlib_SOURCE_DIR})
target_include_directories(zlibstatic PUBLIC ${zlib_BINARY_DIR} ${zlib_SOURCE_DIR})
if(BUILD_SHARED_LIBS)
    target_link_libraries(light_zlib INTERFACE zlib)
else()
    target_link_libraries(light_zlib INTERFACE zlibstatic)
endif()
