cmake_minimum_required(VERSION 3.10)

# Project name
project(motive-pose-stream)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory (optional)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/usr/local/include)

# Source files
set(SOURCES
    /home/ricar/AIMSLab/motive-stream/src/motive-pose-stream.cpp
    /home/ricar/AIMSLab/motive-stream/src/UDPConnection.cpp
)

# Libraries
link_directories(/usr/local/lib)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link external libraries
target_link_libraries(motive-pose-stream vrpn quat pthread)