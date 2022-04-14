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
include_directories(
    ${Vulkan_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIRS}
    External/glm
    Header
)

# Set SOURCES to contain all the source files
set(SOURCES
    # --- Core/Util ---
    Source/Core/Util/FileUtils.cpp
    # --- Core/Vulkan ---
    Source/Core/Vulkan/VulkanGraphicsPipelineBuilder.cpp
    Source/Core/Vulkan/VulkanBuffer.cpp
    Source/Core/Vulkan/VulkanContext.cpp
    Source/Core/Vulkan/VulkanImage.cpp
    Source/Core/Vulkan/VulkanImageView.cpp
    # --- Core ---
    Source/Core/Window.cpp

    Source/Application.cpp
    Source/Input.cpp

    Source/Main.cpp
)

# Executable
add_executable(MapViewer ${SOURCES})

# Link libraries
target_link_libraries(MapViewer ${Vulkan_LIBRARY} glfw ${CMAKE_DL_LIBS})
