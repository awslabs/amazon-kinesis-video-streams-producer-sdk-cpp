cmake_minimum_required (VERSION 2.6)
project (stateTest)
file (GLOB StateTestSources *.cpp)

add_executable(${PROJECT_NAME} ${StateTestSources})
target_link_libraries(${PROJECT_NAME} state)
target_link_libraries(${PROJECT_NAME} client)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
