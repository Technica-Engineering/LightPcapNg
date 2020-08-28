cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(
  zstd
  GIT_REPOSITORY https://github.com/facebook/zstd.git
  GIT_TAG           v1.4.5
)
FetchContent_GetProperties(zstd
    POPULATED ZSTD_POPULATED)
if(NOT ZSTD_POPULATED)
    FetchContent_Populate(zstd)
    add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})
endif()

include_directories(${zstd_SOURCE_DIR}/lib)
