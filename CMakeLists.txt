cmake_minimum_required(VERSION 3.10)

project(light_pcapng C)

# Get dependencies from conan
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(REQUIRES zstd/1.4.0@bincrafters/stable
                BASIC_SETUP)



# Configuration

include_directories("include")

file(GLOB LIGHT_SOURCES src/*.c)

add_library (light_pcapng SHARED ${LIGHT_SOURCES})
add_library (light_pcapng_static STATIC ${LIGHT_SOURCES})

# ZSTD

option(LIGHT_USE_ZSTD "Compile with ZSTD support" ON)

if(LIGHT_USE_ZSTD)
    add_definitions(-DUSE_ZSTD)
    target_link_libraries(light_pcapng ${CONAN_LIBS_ZSTD})
    target_link_libraries(light_pcapng_static ${CONAN_LIBS_ZSTD})
endif()

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})

    include(CTest)
    file(GLOB LIGHT_TESTS tests/*.c)

    foreach(test_file ${LIGHT_TESTS})
        get_filename_component(test_name ${test_file} NAME_WE)
        
        add_executable(${test_name}.test ${test_file})
        target_link_libraries(${test_name}.test light_pcapng_static)

        add_test(
            NAME ${test_name} 
            COMMAND ${test_name}.test "${CMAKE_SOURCE_DIR}/pcaps/caneth.pcapng"
        )
    endforeach()

endif()