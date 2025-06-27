cmake_minimum_required(VERSION 3.10)

# Project name
project(motive-pose-stream)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directory (optional)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add VRPN
add_subdirectory(dependencies/vrpn)

# Add any required defines/options
set(VRPN_USE_HID ON CACHE BOOL "Enable HID support")
set(VRPN_USE_LIBUSB_1_0 ON CACHE BOOL "Enable LibUSB support")

# Source files
set(SOURCES
    src/motive-pose-stream.cpp
    src/UDPConnection.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/dependencies/vrpn/
)


link_directories(/usr/local/lib) #If installed locally
link_directories(${CMAKE_SOURCE_DIR}/dependencies/vrpn/build)



# Link VRPN library target
target_link_libraries(${PROJECT_NAME} PRIVATE vrpn quat pthread)
