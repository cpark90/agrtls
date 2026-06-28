cmake_minimum_required(VERSION 3.0.2)
project(scale_reader)

add_compile_options(-std=c++14)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  message_generation
)

add_message_files(
  FILES
  WeightReading.msg
)

generate_messages(
  DEPENDENCIES
  std_msgs
)

catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS roscpp std_msgs message_runtime
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

add_executable(scale_reader_node src/scale_reader_node.cpp)
add_dependencies(scale_reader_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(scale_reader_node ${catkin_LIBRARIES})

install(TARGETS scale_reader_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
