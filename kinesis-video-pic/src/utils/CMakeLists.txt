cmake_minimum_required(VERSION 2.8)

project(utils)
kinesis_video_library_setup(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
kinesis_video_library_install()
