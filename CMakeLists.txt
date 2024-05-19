cmake_minimum_required(VERSION 3.10)

# set the project name
project(Http2Sandbox)

# specify the C++ standard
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Display current build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# add .cpp files to the executable
file(GLOB cpp_files "src/*.cpp")
add_executable(http2_sandbox ${cpp_files})

