cmake_minimum_required (VERSION 2.6)
project (mkvgenTest)
file (GLOB MkvgenTestSources *.cpp)
file(COPY samples DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} ${MkvgenTestSources})
target_link_libraries(${PROJECT_NAME} mkvgen)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
