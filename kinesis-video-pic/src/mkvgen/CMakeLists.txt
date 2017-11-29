cmake_minimum_required(VERSION 2.8)

project(mkvgen)
kinesis_video_library_setup(${PROJECT_NAME})
target_link_libraries(mkvgen utils)
kinesis_video_library_install()
