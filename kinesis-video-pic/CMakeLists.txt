cmake_minimum_required(VERSION 2.8)

project(PlatformIndependentNativeRepository LANGUAGES CXX)

include_directories(${BP_TESTBUILD_LIBFARM}/include)
link_directories(${BP_TESTBUILD_LIBFARM}/lib)
# link_directories(${BP_BUILD_LIB})
link_directories(${BP_TESTBUILD_LIB})

include_directories(${BP_INCLUDE_DIRS})

# Required for compilation of AL2012
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lrt -lpthread")
endif()

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# Include source base directory to allow absolute header paths
include_directories(${CMAKE_SOURCE_DIR})
enable_testing()
add_subdirectory(src)
