cmake_minimum_required (VERSION 2.6)
project(PlatformIndependentNativeRepository LANGUAGES CXX)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

project (clientTest)
file (GLOB ClientTestSources *.cpp)

add_executable(${PROJECT_NAME} ${ClientTestSources})
target_link_libraries(${PROJECT_NAME} state)
target_link_libraries(${PROJECT_NAME} client)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
