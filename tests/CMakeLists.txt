cmake_minimum_required(VERSION 3.11)

if(MSVC)
    add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Tests
file(GLOB LIGHT_TESTS *.c)

file(GLOB samples_pcapng "../pcaps/*.pcapng")

if(LIGHT_USE_ZSTD)
    file(GLOB samples_zsts "../pcaps/*.zst")
endif()

foreach(test_file ${LIGHT_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
        
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} light_pcapng_static)
endforeach()

foreach(sample ${samples_pcapng} ${samples_zsts})
    get_filename_component(param ${sample} NAME)
    string(REPLACE "." "_" param ${param})
    add_test(
        NAME "blocks.dump.${param}"
        COMMAND test_blocks_dump 
            ${sample} 
            "${CMAKE_CURRENT_LIST_DIR}/results/blocks.dump.${param}.txt"
    )
    add_test(
        NAME "packets.dump.${param}"
        COMMAND test_packets_dump 
            ${sample} 
            "${CMAKE_CURRENT_LIST_DIR}/results/packets.dump.${param}.txt"
    )
endforeach()

add_test(
    NAME "blocks.merge.merge"
    COMMAND test_blocks_merge 
        "${CMAKE_CURRENT_LIST_DIR}/results/blocks.merged.pcapng" 
        ${samples_pcapng}
)

add_test(
    NAME "packets.merge.merge"
    COMMAND test_packets_merge 
        "${CMAKE_CURRENT_LIST_DIR}/results/packets.merged.pcapng" 
        ${samples_pcapng}
)

add_test(
    NAME "unit.io.mem"
    COMMAND test_io_mem "${CMAKE_CURRENT_LIST_DIR}/../pcaps/caneth.pcapng"
)
