cmake_minimum_required(VERSION 2.8)

project(view)
kinesis_video_library_setup(${PROJECT_NAME})
target_link_libraries(view heap utils)
kinesis_video_library_install()
