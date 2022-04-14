cmake_minimum_required(VERSION 3.10)

# Project name
project(MapViewer)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json for YouCompleteMe (YCM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Specify include directories
include_directories(${Vulkan_INCLUDE_DIR} ${GLFW_INCLUDE_DIRS} Header)

# Set SOURCES to contain all the source files
set(SOURCES
    Source/Main.cpp
)

# Executable
add_executable(MapViewer ${SOURCES})

# Link libraries
target_link_libraries(MapViewer ${Vulkan_LIBRARY} glfw ${CMAKE_DL_LIBS})
