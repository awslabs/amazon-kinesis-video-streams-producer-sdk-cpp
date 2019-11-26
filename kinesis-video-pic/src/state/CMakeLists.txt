cmake_minimum_required(VERSION 2.8)

project(state)
kinesis_video_library_setup(${PROJECT_NAME})
target_link_libraries(state mkvgen)
kinesis_video_library_install()
