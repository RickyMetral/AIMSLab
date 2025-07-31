cmake_minimum_required(VERSION 3.15)

# Project name
project(crazyflie-pose-stream)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directory (optional)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


# Enable strict handling of warnings
# add_definitions("-Wall -Wextra -Werror")

# Source files
set(SOURCES
    src/crazyflie-pose-stream.cpp
    src/UDPConnection.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})


# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/dependencies/vrpn/
    # ${CMAKE_SOURCE_DIR}/dependencies/c_library_v2/

)

# Link VRPN library target
target_link_libraries(${PROJECT_NAME} PRIVATE 
    vrpn 
    quat 
    pthread
    # MAVSDK::mavsdk             #All apps link against mavsdk library
    # MAVSDK::mavsdk_action      # If action plugin used by app ...
    # MAVSDK::mavsdk_telemetry   #If telemetry plugin used by app ...
)
