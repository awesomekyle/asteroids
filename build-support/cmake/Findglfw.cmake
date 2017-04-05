set(GLFW_PATH "${CMAKE_SOURCE_DIR}/3rd-party/glfw-3.2.1")

option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)

add_subdirectory(${GLFW_PATH})

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU OR ${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    # GLFW uses deprecated functionality, so warnings are disabled
    target_compile_options(glfw PRIVATE -w)
endif()
