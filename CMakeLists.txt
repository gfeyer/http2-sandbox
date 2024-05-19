cmake_minimum_required(VERSION 3.10)

# set the project name
project(Http2Sandbox)

# specify the C++ standard
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Display current build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Find packages
# find_package(Boost 1.85 REQUIRED COMPONENTS system thread)  # Ensure you have the correct version
find_package(Boost REQUIRED COMPONENTS system thread)

set(OPENSSL_USE_STATIC_LIBS FALSE)
find_package(OpenSSL REQUIRED)

# Include directories for headers
include_directories(${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

# Specify the correct paths for static libraries
link_directories(/opt/vcpkg/installed/x64-linux/lib)

# Source files
file(GLOB cpp_files "src/*.cpp")
add_executable(http2_sandbox ${cpp_files})



# Linking all libraries, ensure static ones are appropriately handled
target_link_libraries(http2_sandbox
    ${Boost_LIBRARIES}
    /opt/vcpkg/installed/x64-linux/lib/libnghttp2_asio.a
    /opt/vcpkg/installed/x64-linux/lib/libnghttp2.a
    /opt/vcpkg/installed/x64-linux/lib/libssl.a
    /opt/vcpkg/installed/x64-linux/lib/libcrypto.a
    # OpenSSL::SSL
    # OpenSSL::Crypto
    pthread
    dl
)


# Additional definitions to handle any Boost auto-linking features
add_definitions(-DBOOST_ALL_NO_LIB)