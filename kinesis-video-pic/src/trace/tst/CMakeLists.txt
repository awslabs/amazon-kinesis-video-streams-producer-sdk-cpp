cmake_minimum_required (VERSION 2.6)
project (traceTest)
file (GLOB TraceTestSources *.cpp)

add_executable(${PROJECT_NAME} ${TraceTestSources})
target_link_libraries(${PROJECT_NAME} trace)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
