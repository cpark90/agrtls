cmake_minimum_required(VERSION 3.0.2)
project(harvest_recorder)

add_compile_options(-std=c++14)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  message_generation
  message_filters
  uwb_positioning
  scale_reader
)

add_message_files(
  FILES
  HarvestReading.msg
)

generate_messages(
  DEPENDENCIES
  std_msgs
  uwb_positioning
  scale_reader
)

catkin_package(
  CATKIN_DEPENDS roscpp std_msgs message_runtime message_filters uwb_positioning scale_reader
)

include_directories(${catkin_INCLUDE_DIRS})

add_executable(recorder_node src/recorder_node.cpp)
add_dependencies(recorder_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(recorder_node ${catkin_LIBRARIES})

add_executable(sim_harvest src/sim_harvest.cpp)
add_dependencies(sim_harvest ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(sim_harvest ${catkin_LIBRARIES})

install(TARGETS recorder_node sim_harvest
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
