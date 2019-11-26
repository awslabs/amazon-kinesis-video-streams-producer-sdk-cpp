cmake_minimum_required (VERSION 2.6)

project (durationTest)
file (GLOB DurationTestSources *.cpp)

add_executable(${PROJECT_NAME} ${DurationTestSources})

target_link_libraries(${PROJECT_NAME} duration)
target_link_libraries(${PROJECT_NAME} utils)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
