// Scale reader node.
//
// Opens a serial port connected to a weight scale, parses each frame,
// and publishes WeightReading messages.
//
// STUB: parseFrame() assumes the scale outputs one ASCII float per line
// (e.g. "72.450\n"). Replace once the actual protocol is confirmed.
#include <ros/ros.h>
#include <scale_reader/WeightReading.h>
#include <scale_reader/serial_port.h>

#include <string>

namespace {

// STUB: expects one ASCII float per line, e.g. "72.450".
// Returns false to drop malformed frames.
bool parseFrame(const std::string& raw, double& weight) {
  try {
    weight = std::stod(raw);
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

}  // namespace

int main(int argc, char** argv) {
  ros::init(argc, argv, "scale_reader_node");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  std::string port, frame_id, topic, unit;
  int baud = 9600;
  double read_timeout = 1.0;

  if (!pnh.getParam("port", port)) {
    ROS_FATAL("missing required param: ~port");
    return 1;
  }
  pnh.param<std::string>("topic",    topic,    "/scale/weight");
  pnh.param<std::string>("frame_id", frame_id, "scale");
  pnh.param<std::string>("unit",     unit,     "kg");
  pnh.param("baud",         baud,         9600);
  pnh.param("read_timeout", read_timeout, 1.0);

  ros::Publisher pub = nh.advertise<scale_reader::WeightReading>(topic, 100);

  scale_reader::SerialPort sp;
  try {
    sp.open(port, baud);
  } catch (const std::exception& e) {
    ROS_FATAL("cannot open %s: %s", port.c_str(), e.what());
    return 1;
  }
  ROS_INFO("scale_reader_node: %s @ %d baud -> %s", port.c_str(), baud, topic.c_str());

  std::string line;
  double weight = 0.0;
  while (ros::ok()) {
    if (!sp.readLine(line, read_timeout)) continue;
    if (line.empty()) continue;
    if (!parseFrame(line, weight)) {
      ROS_WARN_THROTTLE(5.0, "unparseable scale frame: %s", line.c_str());
      continue;
    }
    scale_reader::WeightReading msg;
    msg.header.stamp    = ros::Time::now();
    msg.header.frame_id = frame_id;
    msg.weight = weight;
    msg.unit   = unit;
    pub.publish(msg);
    ros::spinOnce();
  }

  sp.close();
  return 0;
}
