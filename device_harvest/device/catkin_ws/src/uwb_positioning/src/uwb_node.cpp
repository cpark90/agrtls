// UWB positioning node.
//
// Wires together the modular components:
//   serial_port  →  frame_parser  →  AnchorMap
//                                  →  trilateration (init)
//                                  →  PositionFilter (predict + updateRange)
//                                  →  UwbPosition publisher
//
// To swap the position estimator set filter/type in params.yaml and add
// a case to makeFilter() — nothing else in this file changes.
#include <ros/ros.h>
#include <uwb_positioning/UwbPosition.h>

#include <uwb_positioning/anchor_map.h>
#include <uwb_positioning/frame_parser.h>
#include <uwb_positioning/position_filter.h>
#include <uwb_positioning/serial_port.h>
#include <uwb_positioning/trilateration.h>
#include <uwb_positioning/uwb_ekf.h>

#include <memory>
#include <string>
#include <vector>

namespace {

// ---- Filter factory ----------------------------------------------------------
// Add new filter types here.  The node only sees PositionFilter*.
std::unique_ptr<uwb_positioning::PositionFilter>
makeFilter(ros::NodeHandle& pnh) {
  std::string type;
  pnh.param<std::string>("filter/type", type, "ekf");

  if (type == "ekf") {
    uwb_positioning::EkfConfig cfg;
    pnh.param("ekf/sigma_a",     cfg.sigma_a,     0.3);
    pnh.param("ekf/sigma_r",     cfg.sigma_r,     0.10);
    pnh.param("ekf/rx_ref_dbm",  cfg.rx_ref_dbm, -70.0);
    pnh.param("ekf/rx_decay_db", cfg.rx_decay_db, 20.0);
    return std::unique_ptr<uwb_positioning::PositionFilter>(
        new uwb_positioning::UwbEkf(cfg));
  }

  throw std::runtime_error("uwb_node: unknown filter type '" + type +
                           "'. Supported: ekf");
}

// ---- Anchor table from params ------------------------------------------------
// params.yaml format:
//   anchors:
//     - {id: "A0", x: 0.0, y: 0.0, z: 0.0}
uwb_positioning::AnchorMap loadAnchors(ros::NodeHandle& pnh) {
  uwb_positioning::AnchorMap map;
  XmlRpc::XmlRpcValue raw;
  if (!pnh.getParam("anchors", raw) ||
      raw.getType() != XmlRpc::XmlRpcValue::TypeArray) {
    return map;
  }
  for (int i = 0; i < raw.size(); ++i) {
    uwb_positioning::Anchor a;
    a.id = static_cast<std::string>(raw[i]["id"]);
    a.x  = static_cast<double>(raw[i]["x"]);
    a.y  = static_cast<double>(raw[i]["y"]);
    a.z  = static_cast<double>(raw[i]["z"]);
    map.add(a);
  }
  return map;
}

// ---- Per-anchor measurement cache for trilateration init ---------------------
struct AnchorMeas {
  double range      = 0.0;
  double rx_power   = 0.0;
  bool   valid      = false;
};

// Attempt trilateration over all currently valid anchors.
// Returns true and writes (x,y,z) when geometry is non-degenerate.
bool tryTrilaterate(const uwb_positioning::AnchorMap& anchors,
                    const std::vector<AnchorMeas>&    cache,
                    double rx_ref_dbm, double rx_decay_db, double sigma_r,
                    double& x_out, double& y_out, double& z_out) {
  std::vector<uwb_positioning::Anchor> va;
  std::vector<double> vr, vw;
  for (size_t i = 0; i < cache.size(); ++i) {
    if (!cache[i].valid) continue;
    va.push_back(anchors.at(i));
    vr.push_back(cache[i].range);
    vw.push_back(uwb_positioning::rxToWeight(
        cache[i].rx_power, rx_ref_dbm, rx_decay_db, sigma_r));
  }
  if (va.size() < 3) return false;
  return uwb_positioning::trilaterate(va, vr, vw, x_out, y_out, z_out);
}

}  // namespace

// ---- Main -------------------------------------------------------------------
int main(int argc, char** argv) {
  ros::init(argc, argv, "uwb_node");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  // ---- Basic params ----
  std::string port, frame_id, topic;
  int    baud            = 115200;
  double read_timeout    = 1.0;
  double timestamp_scale = 1e-3;  // raw device timestamp → seconds

  if (!pnh.getParam("port", port)) {
    ROS_FATAL("uwb_node: missing required param ~port");
    return 1;
  }
  pnh.param<std::string>("topic",             topic,          "/uwb/position");
  pnh.param<std::string>("frame_id",          frame_id,       "uwb");
  pnh.param("baud",                           baud,           115200);
  pnh.param("read_timeout",                   read_timeout,   1.0);
  pnh.param("ekf/timestamp_scale",            timestamp_scale, 1e-3);

  // ---- Anchor table ----
  const uwb_positioning::AnchorMap anchors = loadAnchors(pnh);
  if (anchors.empty()) {
    ROS_FATAL("uwb_node: no anchors configured — set ~anchors in params.yaml");
    return 1;
  }
  ROS_INFO("uwb_node: %zu anchor(s) loaded", anchors.size());

  // ---- Position filter ----
  std::unique_ptr<uwb_positioning::PositionFilter> filter;
  try {
    filter = makeFilter(pnh);
  } catch (const std::exception& e) {
    ROS_FATAL("%s", e.what());
    return 1;
  }

  // Noise params needed for trilateration weights (must match filter's EkfConfig).
  double sigma_r, rx_ref_dbm, rx_decay_db;
  pnh.param("ekf/sigma_r",     sigma_r,     0.10);
  pnh.param("ekf/rx_ref_dbm",  rx_ref_dbm, -70.0);
  pnh.param("ekf/rx_decay_db", rx_decay_db, 20.0);

  // ---- Measurement cache (for trilateration-based init) ----
  std::vector<AnchorMeas>  meas_cache(anchors.size());
  std::vector<double>      last_ranges(anchors.size(), 0.0);

  // ---- Publisher ----
  ros::Publisher pub = nh.advertise<uwb_positioning::UwbPosition>(topic, 100);

  // ---- Serial port ----
  uwb_positioning::SerialPort sp;
  try {
    sp.open(port, baud);
  } catch (const std::exception& e) {
    ROS_FATAL("uwb_node: cannot open %s: %s", port.c_str(), e.what());
    return 1;
  }
  ROS_INFO("uwb_node: %s @ %d baud → %s", port.c_str(), baud, topic.c_str());

  // ---- Main loop ----
  std::string line;
  while (ros::ok()) {
    if (!sp.readLine(line, read_timeout)) continue;
    if (line.empty()) continue;

    // Parse frame
    uwb_positioning::UwbFrame frame;
    if (!uwb_positioning::parseFrame(line, frame)) {
      ROS_WARN_THROTTLE(5.0, "uwb_node: unparseable frame: %s", line.c_str());
      continue;
    }

    // Look up anchor
    size_t idx;
    if (!anchors.find(frame.anchor_id, idx)) {
      ROS_WARN_THROTTLE(10.0, "uwb_node: unknown anchor '%s' — add to params.yaml",
                        frame.anchor_id.c_str());
      continue;
    }

    // Device clock time in seconds (used for EKF predict dt)
    const double t_device = frame.device_timestamp * timestamp_scale;

    // Update cache
    meas_cache[idx] = {frame.range_m, frame.rx_power_dbm, true};
    last_ranges[idx] = frame.range_m;

    // ---- Initialise filter via trilateration ----
    if (!filter->isInitialized()) {
      const size_t n_valid = static_cast<size_t>(
          std::count_if(meas_cache.begin(), meas_cache.end(),
                        [](const AnchorMeas& m) { return m.valid; }));

      if (n_valid < 3) {
        ROS_INFO_THROTTLE(2.0, "uwb_node: waiting for >= 3 anchors (%zu/%zu)",
                          n_valid, anchors.size());
        continue;
      }

      double ix, iy, iz;
      if (!tryTrilaterate(anchors, meas_cache,
                          rx_ref_dbm, rx_decay_db, sigma_r,
                          ix, iy, iz)) {
        ROS_WARN_THROTTLE(2.0, "uwb_node: trilateration failed — check anchor positions");
        continue;
      }

      filter->init(ix, iy, iz, t_device);
      ROS_INFO("uwb_node: filter initialised at (%.3f, %.3f, %.3f)", ix, iy, iz);
      continue;
    }

    // ---- Predict + sequential update ----
    filter->predict(t_device);
    filter->updateRange(anchors.at(idx), frame.range_m, frame.rx_power_dbm);

    // ---- Publish ----
    double px, py, pz;
    filter->getPosition(px, py, pz);
    const auto cov = filter->positionCovariance();

    uwb_positioning::UwbPosition msg;
    msg.header.stamp    = ros::Time::now();
    msg.header.frame_id = frame_id;
    msg.x = px;
    msg.y = py;
    msg.z = pz;
    for (size_t i = 0; i < 9; ++i)
      msg.position_covariance[i] = cov[i];
    msg.anchor_distances = last_ranges;

    pub.publish(msg);
    ros::spinOnce();
  }

  sp.close();
  return 0;
}
