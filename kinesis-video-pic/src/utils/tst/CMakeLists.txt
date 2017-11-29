cmake_minimum_required (VERSION 2.6)
project (utilsTest)
file (GLOB UtilsTestSources *.cpp)

add_executable(${PROJECT_NAME} ${UtilsTestSources})
target_link_libraries(${PROJECT_NAME} utils)
target_link_libraries(${PROJECT_NAME} gtest)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
