cmake_minimum_required(VERSION 3.0.2)
project(uwb_positioning)

add_compile_options(-std=c++14)

find_package(Eigen3 REQUIRED)
find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  message_generation
)

add_message_files(
  FILES
  UwbPosition.msg
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
  ${EIGEN3_INCLUDE_DIR}
)

add_executable(uwb_node src/uwb_node.cpp)
add_dependencies(uwb_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(uwb_node ${catkin_LIBRARIES})

install(TARGETS uwb_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
