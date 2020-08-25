cmake_minimum_required (VERSION 2.6)
project (light_pcapng C)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/include/ -Wall -O2 -fPIC -DUNIVERSAL -g")

file(GLOB LIGHT_SOURCES src/*.c)

add_library (light_pcapng SHARED ${LIGHT_SOURCES})

add_library (light_pcapng_static STATIC ${LIGHT_SOURCES})

add_executable (test_read.test
    src/tests/test_read.c
    )
target_link_libraries(test_read.test light_pcapng_static)

add_executable (test_read_write.test
    src/tests/test_read_write.c
    )
target_link_libraries(test_read_write.test light_pcapng_static)

add_executable (test_mem.test
    src/tests/test_mem.c
    )
target_link_libraries(test_mem.test light_pcapng_static)

add_executable (test_subcapture.test
    src/tests/test_subcapture.c
    )
target_link_libraries(test_subcapture.test light_pcapng_static)

add_executable (test_feature.test
    src/tests/test_feature.c
    )
target_link_libraries(test_feature.test light_pcapng_static)

add_executable (test_read_packets.test
    src/tests/test_read_packets.c
    )
target_link_libraries(test_read_packets.test light_pcapng_static)

add_executable (test_read_write_packets.test
    src/tests/test_read_write_packets.c
    )
target_link_libraries(test_read_write_packets.test light_pcapng_static)
