// UWB positioning node.
//
// Reads anchor-distance frames from a UWB32 module over serial, computes the
// tag position via multilateration, and publishes UwbPosition.
//
// STUBS:
//   parseFrame()     — frame format is unknown; replace once protocol is confirmed.
//   multilaterate()  — returns (0,0,0) until anchor positions are set in params.yaml
//                      and the algorithm is implemented (linear LS recommended).
#include <ros/ros.h>
#include <uwb_positioning/UwbPosition.h>
#include <uwb_positioning/serial_port.h>

#include <sstream>
#include <string>
#include <vector>
#include <cmath>

namespace {

// STUB: expects one ASCII line of comma-separated distances, e.g. "1.23,4.56,7.89,10.1".
// Each value is the measured distance (m) to the corresponding anchor.
// Returns false to drop malformed frames.
bool parseFrame(const std::string& raw, std::vector<double>& distances) {
  distances.clear();
  std::stringstream ss(raw);
  std::string tok;
  while (std::getline(ss, tok, ',')) {
    try {
      distances.push_back(std::stod(tok));
    } catch (const std::exception&) {
      return false;
    }
  }
  return !distances.empty();
}

struct Vec3 { double x, y, z; };

// STUB: linear least-squares multilateration.
// TODO: set anchor positions in params.yaml, then implement the linearised LS solver.
// For 2D (all z == 0) at least 3 anchors are needed; 4 for 3D.
bool multilaterate(const std::vector<Vec3>& anchors,
                   const std::vector<double>& dists,
                   Vec3& out) {
  if (anchors.size() < 3 || anchors.size() != dists.size()) return false;
  out = {0.0, 0.0, 0.0};  // replace with real solver
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  ros::init(argc, argv, "uwb_node");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  std::string port, frame_id, topic;
  int baud = 115200;
  double read_timeout = 1.0;

  if (!pnh.getParam("port", port)) {
    ROS_FATAL("missing required param: ~port");
    return 1;
  }
  pnh.param<std::string>("topic",    topic,    "/uwb/position");
  pnh.param<std::string>("frame_id", frame_id, "uwb");
  pnh.param("baud",         baud,         115200);
  pnh.param("read_timeout", read_timeout, 1.0);

  // Anchor positions from params.yaml: anchors: [[x0,y0,z0], [x1,y1,z1], ...]
  std::vector<Vec3> anchors;
  XmlRpc::XmlRpcValue anchor_list;
  if (pnh.getParam("anchors", anchor_list)) {
    for (int i = 0; i < anchor_list.size(); ++i) {
      Vec3 p;
      p.x = static_cast<double>(anchor_list[i][0]);
      p.y = static_cast<double>(anchor_list[i][1]);
      p.z = static_cast<double>(anchor_list[i][2]);
      anchors.push_back(p);
    }
  }
  if (anchors.empty())
    ROS_WARN("no anchor positions configured — position will be (0,0,0)");

  ros::Publisher pub = nh.advertise<uwb_positioning::UwbPosition>(topic, 100);

  uwb_positioning::SerialPort sp;
  try {
    sp.open(port, baud);
  } catch (const std::exception& e) {
    ROS_FATAL("cannot open %s: %s", port.c_str(), e.what());
    return 1;
  }
  ROS_INFO("uwb_node: %s @ %d baud -> %s", port.c_str(), baud, topic.c_str());

  std::string line;
  std::vector<double> dists;
  while (ros::ok()) {
    if (!sp.readLine(line, read_timeout)) continue;
    if (line.empty()) continue;
    if (!parseFrame(line, dists)) {
      ROS_WARN_THROTTLE(5.0, "unparseable UWB frame: %s", line.c_str());
      continue;
    }

    uwb_positioning::UwbPosition msg;
    msg.header.stamp    = ros::Time::now();
    msg.header.frame_id = frame_id;
    msg.anchor_distances = dists;

    Vec3 pos;
    if (!multilaterate(anchors, dists, pos))
      ROS_WARN_THROTTLE(10.0, "multilateration failed — set anchor positions in params.yaml");
    msg.x = pos.x;
    msg.y = pos.y;
    msg.z = pos.z;

    pub.publish(msg);
    ros::spinOnce();
  }

  sp.close();
  return 0;
}
