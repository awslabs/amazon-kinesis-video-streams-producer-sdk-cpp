cmake_minimum_required (VERSION 2.6)

project (heapTest)
file (GLOB HeapTestSources *.cpp)

add_executable(${PROJECT_NAME} ${HeapTestSources})

target_link_libraries(${PROJECT_NAME} heap)
target_link_libraries(${PROJECT_NAME} utils)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
