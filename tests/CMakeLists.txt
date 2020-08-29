cmake_minimum_required(VERSION 3.11)

# Dr Memory
find_program(LIGHT_DRMEMORY_PATH "drmemory" DOC "Path to Dr. Memory binary")
if(LIGHT_DRMEMORY_PATH)
    # Needed so Dr. Memory can work correctly
    # See https://dynamorio.org/drmemory_docs/page_prep.html
    string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
endif()

# Tests
file(GLOB LIGHT_TESTS *.c)

set(sample_file, "${CMAKE_CURRENT_LIST_DIR}/../pcaps/caneth.pcapng")

foreach(test_file ${LIGHT_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
        
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} light_pcapng_static)

    add_test(
        NAME ${test_name} 
        COMMAND ${test_name} ${sample_file}
    )
    if(LIGHT_DRMEMORY_PATH)
        add_test(
            NAME ${test_name}_drmemory
            COMMAND "${LIGHT_DRMEMORY_PATH}"
                "-batch"
                "-exit_code_if_errors" "99"
                $<TARGET_FILE:${test_name}> ${sample_file}
        ) 
    endif()

endforeach()