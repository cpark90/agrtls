// Hardware-free simulator for harvest_recorder testing.
//
// Publishes fake UwbPosition and WeightReading at a fixed rate so recorder_node
// can be exercised without real devices attached.
//
// Usage (inside container):
//   roscore &
//   rosrun harvest_recorder recorder_node &
//   rosrun harvest_recorder sim_harvest
#include <ros/ros.h>
#include <uwb_positioning/UwbPosition.h>
#include <scale_reader/WeightReading.h>

int main(int argc, char** argv) {
  ros::init(argc, argv, "sim_harvest");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  double rate_hz = 10.0;
  pnh.param("rate", rate_hz, 10.0);

  ros::Publisher pub_uwb = nh.advertise<uwb_positioning::UwbPosition>(
      "/uwb/position", 10);
  ros::Publisher pub_scale = nh.advertise<scale_reader::WeightReading>(
      "/scale/weight", 10);

  ros::Rate rate(rate_hz);
  int i = 0;
  while (ros::ok()) {
    const ros::Time now = ros::Time::now();

    uwb_positioning::UwbPosition uwb;
    uwb.header.stamp    = now;
    uwb.header.frame_id = "uwb";
    uwb.x = 1.0 + 0.01 * i;
    uwb.y = 2.0;
    uwb.z = 0.0;
    uwb.anchor_distances = {1.5, 2.5, 3.0, 2.0};
    pub_uwb.publish(uwb);

    scale_reader::WeightReading weight;
    weight.header.stamp    = now;
    weight.header.frame_id = "scale";
    weight.weight = 10.0 + 0.1 * (i % 10);
    weight.unit   = "kg";
    pub_scale.publish(weight);

    ++i;
    ros::spinOnce();
    rate.sleep();
  }
  return 0;
}
