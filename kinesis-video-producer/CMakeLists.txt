cmake_minimum_required(VERSION 3.1)

project(KinesisVideoProducer LANGUAGES CXX)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++14 ")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lrt -lpthread")
endif()


include_directories(${BP_BUILD_INCLUDE_DIRS})
include_directories(${BP_TESTBUILD_INCLUDE_DIRS})

link_directories(${BP_BUILD_LIB})
link_directories(${BP_TESTBUILD_LIB})

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# Include source base directory to allow absolute header paths
include_directories(${CMAKE_SOURCE_DIR})

enable_testing()
add_subdirectory(src)

add_subdirectory(tst)
