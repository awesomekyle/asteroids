set(CATCH_PATH ${CMAKE_SOURCE_DIR}/3rd-party/Catch-1.9.3)

add_library(catch STATIC ${CATCH_PATH}/catch.cpp)
target_include_directories(catch
    PUBLIC
        ${CATCH_PATH}
)
set_default_target_folder(catch)

function(igpa_add_catchtest target)
    igpa_add_executable(${target} ENABLE_RTTI_EXCEPTIONS ${ARGN})
    target_link_libraries(${target} PRIVATE catch)

    if(CI_BUILD)
        add_test(NAME ${target}
                 COMMAND ${target}  "--out=${CMAKE_BINARY_DIR}/catch_${target}_results.xml")
        set_target_properties(${target}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests"
                RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tests"
                RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests"
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
    else()
        add_custom_command(TARGET ${target}
                           POST_BUILD
                           COMMAND ${target}
                           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                           COMMENT "Running ${target}..." VERBATIM)
    endif()
endfunction()
