cmake_minimum_required (VERSION 2.6)
project (clientTest)
file (GLOB ClientTestSources *.cpp)

add_executable(${PROJECT_NAME} ${ClientTestSources})
target_link_libraries(${PROJECT_NAME} client)
target_link_libraries(${PROJECT_NAME} gtest)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
