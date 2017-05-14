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
if(Vulkan_FOUND)
    set(HAVE_VULKAN 1)
endif()
include(CheckIncludeFiles)

find_library(D3D12_LIB D3D12)
if(D3D12_LIB)
    set(HAVE_D3D12 1)
endif()

find_library(METAL_LIB Metal)
if (METAL_LIB)
    find_library(QUARTZCORE_LIB QuartzCore)
    set(HAVE_METAL 1)
endif()

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

######################
# Source             #
######################
add_subdirectory(src)
