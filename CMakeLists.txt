cmake_minimum_required(VERSION 3.24)
# Set the project name
project(MyProject)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Separate .cpp and .h files into different groups
file(GLOB_RECURSE CPP_SOURCES thread_pool/*.cpp)
file(GLOB_RECURSE HEADER_SOURCES thread_pool/*.h)

# Exclude CMake's compiler identification files from being part of the build
list(FILTER CPP_SOURCES EXCLUDE REGEX "CMakeCXXCompilerId.cpp")

# Add an executable target named "my_executable" using the collected .cpp sources
add_executable(my_executable ${CPP_SOURCES} ${HEADER_SOURCES})

# Create source groups for Visual Studio
source_group("Source Files" FILES ${CPP_SOURCES})
source_group("Header Files" FILES ${HEADER_SOURCES})

# If your files are in subdirectories, this will organize them accordingly
foreach(_source IN ITEMS ${CPP_SOURCES})
    get_filename_component(_source_path "${_source}" PATH)
    source_group("Source Files\\${_source_path}" FILES "${_source}")
endforeach()

foreach(_header IN ITEMS ${HEADER_SOURCES})
    get_filename_component(_header_path "${_header}" PATH)
    source_group("Header Files\\${_header_path}" FILES "${_header}")
endforeach()

# Optionally, specify target sources for other IDEs
target_sources(my_executable PRIVATE ${CPP_SOURCES} PUBLIC ${HEADER_SOURCES})
