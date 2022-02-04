cmake_minimum_required(VERSION 3.22)
project(sys_imgui CXX C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(IMGUI_STATIC true)

# We have to add the cmake directory to have access to
# the Switch tools and also find_package for libnx.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

## Required libraries and utilities

# SwitchTools provide various helper functions for building Switch file formats.
include(SwitchTools)

# Find libnx, which is required by our application.
find_package(LIBNX REQUIRED)

## Targets

# First, we want to add the regular ELF target.
add_executable(sys_imgui source/init.cpp source/ImguiService.cpp source/ImguiService.hpp)

# Next, we need to set the required target properties
# so CMake can detect the JSON configuration for the module.
set_target_properties(sys_imgui PROPERTIES
        CONFIG_JSON "${CMAKE_CURRENT_LIST_DIR}/sys-imgui.json")

# Then, we link the required libraries (libnx) against the executable.
target_include_directories(sys_imgui PRIVATE bindingGen/cimgui)
target_link_libraries(sys_imgui switch::libnx cimgui)


# And finally we can add the actual sysmodule (.nsp) target.
add_nsp_target(sys_imgui)
