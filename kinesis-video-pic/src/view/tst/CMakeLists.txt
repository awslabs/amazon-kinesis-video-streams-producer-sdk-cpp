cmake_minimum_required (VERSION 2.6)
project (viewTest)
file (GLOB ViewTestSources *.cpp)

add_executable(${PROJECT_NAME} ${ViewTestSources})
target_link_libraries(${PROJECT_NAME} view)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
