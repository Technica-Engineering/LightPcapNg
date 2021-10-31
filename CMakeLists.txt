cmake_minimum_required(VERSION 3.13)

project(light_pcapng C)

# Configuration

file(GLOB LIGHT_SOURCES "src/*.c")
file(GLOB LIGHT_HEADERS "include/*.h")

add_library(light_pcapng ${LIGHT_SOURCES})
set_target_properties(light_pcapng PROPERTIES PUBLIC_HEADER
                                              "${LIGHT_HEADERS}")

target_include_directories(light_pcapng PUBLIC "include")
# Export API symbols when the library is created

if(BUILD_SHARED_LIBS)
  target_compile_definitions(light_pcapng PRIVATE LIGHT_EXPORTS=1)
  target_compile_definitions(light_pcapng PUBLIC LIGHT_IMPORTS=1)
endif()

# ZSTD

option(LIGHT_USE_ZSTD "Compile with ZSTD support" ON)

if(LIGHT_USE_ZSTD)
  include(cmake/zstd.cmake)
  add_definitions(-DLIGHT_USE_ZSTD)
  target_link_libraries(light_pcapng light_zstd)
endif()

# ZLIB

option(LIGHT_USE_ZLIB "Compile with ZLIB support" ON)

if(LIGHT_USE_ZLIB)
  include(cmake/zlib.cmake)
  add_definitions(-DLIGHT_USE_ZLIB)
  target_link_libraries(light_pcapng light_zlib)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # Testing
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()

  if(BUILD_COVERAGE)
    target_compile_options(light_pcapng PUBLIC
      -O0        # no optimization
      -g         # generate debug info
      --coverage # sets all required flags
    )
    target_link_options(light_pcapng PUBLIC --coverage)
  endif()

  # We compile with warnings as error on non Windows, We will make it even more
  # strict in the future
  if(MSVC)
    target_compile_options(light_pcapng PRIVATE /W4)
  else()
    target_compile_options(light_pcapng PRIVATE -Werror)
  endif()
endif()

include(GNUInstallDirs)
install(TARGETS light_pcapng PUBLIC_HEADER DESTINATION include)
