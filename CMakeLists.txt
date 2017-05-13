#####################
# Global config     #
#####################
cmake_minimum_required(VERSION 3.7) # For FindVulkan

project(asteroids VERSION 0.0.1)

include(CMakeToolsHelpers OPTIONAL)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/build-support/cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(igpa-targets)

######################
# 3rd party source   #
######################
find_package(glfw)
find_package(catch)
find_package(d3dx12)

######################
# Configuration      #
######################
find_package(Vulkan)
if(NOT Vulkan_FOUND)
    message(WARNING "Vulkan not found on system. Building without Vulkan support")
else()
    set(HAVE_VULKAN 1)
endif()
include(CheckIncludeFiles)
check_include_files(pthread.h HAVE_PTHREAD_H)
check_include_files(d3d12.h HAVE_D3D12)
configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

######################
# Source             #
######################
add_subdirectory(src)
