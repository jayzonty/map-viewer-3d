#!/bin/sh

# Create build directory if it doesn't exist yet
if [ ! -d ./build ]; then
    mkdir build
fi

# CMake
cmake -B ./build .

# Copy compile_commands.json from the build folder to the working directory
cp ./build/compile_commands.json ./compile_commands.json

cd build
make

cd ..
glslangValidator -S vert -e main -o Resources/Shaders/basic_vert.spv -V Resources/Shaders/basic_vert.glsl
glslangValidator -S frag -e main -o Resources/Shaders/basic_frag.spv -V Resources/Shaders/basic_frag.glsl
glslangValidator -S vert -e main -o Resources/Shaders/shadow_vert.spv -V Resources/Shaders/shadow_vert.glsl
glslangValidator -S frag -e main -o Resources/Shaders/shadow_frag.spv -V Resources/Shaders/shadow_frag.glsl
cp -r Resources build/
