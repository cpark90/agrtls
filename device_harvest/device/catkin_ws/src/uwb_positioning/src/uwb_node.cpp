// UWB positioning node — tightly coupled EKF on raw range measurements.
//
// Serial frame format (one per line):
//   "<device_id>,<range_m>,<rx_power_dbm>,<timestamp>"
//   e.g.  "A0,1.234,-78.5,1023456"
//
// timestamp: raw device counter value.  Unit is device-dependent (ms, µs, etc.).
// Set ~ekf/timestamp_scale to convert to seconds (default 1e-3 for ms).
// The scaled value is used for EKF predict so that dt reflects the device clock,
// not ROS arrival jitter.  header.stamp still uses ros::Time::now() until the
// device epoch is confirmed and a clock-offset calibration is done.
//
// Startup sequence:
//   1. Accumulate one measurement per anchor.
//   2. Run weighted trilateration → initial position.
//   3. Initialise EKF from that position.
//   4. Each subsequent frame:  predict to now → sequential EKF update → publish.
//
// All parameters are in config/params.yaml; nothing is hardcoded here.
#include <ros/ros.h>
#include <uwb_positioning/UwbPosition.h>
#include <uwb_positioning/serial_port.h>
#include <uwb_positioning/uwb_ekf.h>

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// Parse one ASCII frame: "<id>,<range_m>,<rx_power_dbm>,<timestamp>"
// device_ts: raw timestamp value from the device (unit depends on firmware).
// Returns false to drop malformed lines.
bool parseFrame(const std::string& line,
                std::string& id, double& range,
                double& rx_power, double& device_ts) {
  std::istringstream ss(line);
  std::string tok;
  if (!std::getline(ss, tok, ',') || tok.empty()) return false;
  id = tok;
  if (!std::getline(ss, tok, ',')) return false;
  try { range = std::stod(tok); } catch (...) { return false; }
  if (!std::getline(ss, tok, ',')) return false;
  try { rx_power = std::stod(tok); } catch (...) { return false; }
  if (!std::getline(ss, tok, ',')) return false;
  try { device_ts = std::stod(tok); } catch (...) { return false; }
  return range > 0.0;
}

// rx_power (dBm) → trilateration weight = 1 / sigma_r^2
// Uses the same noise model as the EKF so weights are consistent.
double rxToWeight(double rx_power_dbm, double rx_ref_dbm,
                  double rx_decay_db, double sigma_r) {
  const double loss_db    = std::max(0.0, rx_ref_dbm - rx_power_dbm);
  const double noise_gain = std::pow(10.0, loss_db / rx_decay_db);
  const double sigma      = sigma_r * noise_gain;
  return 1.0 / (sigma * sigma);
}

}  // namespace

int main(int argc, char** argv) {
  ros::init(argc, argv, "uwb_node");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  // ---- Basic params ----
  std::string port, frame_id, topic;
  int    baud         = 115200;
  double read_timeout = 1.0;

  if (!pnh.getParam("port", port)) {
    ROS_FATAL("uwb_node: missing required param ~port");
    return 1;
  }
  pnh.param<std::string>("topic",        topic,        "/uwb/position");
  pnh.param<std::string>("frame_id",     frame_id,     "uwb");
  pnh.param("baud",         baud,         115200);
  pnh.param("read_timeout", read_timeout, 1.0);

  // ---- EKF / noise params ----
  double sigma_a, sigma_r, rx_ref_dbm, rx_decay_db, timestamp_scale;
  pnh.param("ekf/sigma_a",         sigma_a,          0.3);
  pnh.param("ekf/sigma_r",         sigma_r,          0.10);
  pnh.param("ekf/rx_ref_dbm",      rx_ref_dbm,      -70.0);
  pnh.param("ekf/rx_decay_db",     rx_decay_db,      20.0);
  pnh.param("ekf/timestamp_scale", timestamp_scale,   1e-3);  // ms → s by default

  // ---- Anchor table ----
  // params.yaml:
  //   anchors:
  //     - {id: "A0", x: 0.0, y: 0.0, z: 0.0}
  //     - {id: "A1", x: 3.0, y: 0.0, z: 0.0}
  std::vector<uwb_positioning::Anchor> anchor_list;
  std::unordered_map<std::string, size_t> anchor_idx;  // id → index in anchor_list

  XmlRpc::XmlRpcValue raw_anchors;
  if (pnh.getParam("anchors", raw_anchors) &&
      raw_anchors.getType() == XmlRpc::XmlRpcValue::TypeArray) {
    for (int i = 0; i < raw_anchors.size(); ++i) {
      uwb_positioning::Anchor a;
      a.id = static_cast<std::string>(raw_anchors[i]["id"]);
      a.x  = static_cast<double>(raw_anchors[i]["x"]);
      a.y  = static_cast<double>(raw_anchors[i]["y"]);
      a.z  = static_cast<double>(raw_anchors[i]["z"]);
      anchor_idx[a.id] = anchor_list.size();
      anchor_list.push_back(a);
    }
  }
  if (anchor_list.empty()) {
    ROS_FATAL("uwb_node: no anchors configured — set ~anchors in params.yaml");
    return 1;
  }
  ROS_INFO("uwb_node: %zu anchors configured", anchor_list.size());

  // ---- EKF setup ----
  uwb_positioning::UwbEkf ekf;
  ekf.configure(sigma_a, sigma_r, rx_ref_dbm, rx_decay_db);

  // Per-anchor measurement cache: latest (range, rx_power) for each anchor.
  // Used to accumulate readings for trilateration-based initialisation.
  struct Meas { double range; double rx_power; bool valid = false; };
  std::vector<Meas> cache(anchor_list.size());

  // Latest range per anchor for the published message field anchor_distances[].
  std::vector<double> last_ranges(anchor_list.size(), 0.0);

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
  ROS_INFO("uwb_node: %s @ %d baud -> %s", port.c_str(), baud, topic.c_str());

  // ---- Main loop ----
  std::string line;
  while (ros::ok()) {
    if (!sp.readLine(line, read_timeout)) continue;
    if (line.empty()) continue;

    std::string id;
    double range, rx_power, device_ts_raw;
    if (!parseFrame(line, id, range, rx_power, device_ts_raw)) {
      ROS_WARN_THROTTLE(5.0, "uwb_node: unparseable frame: %s", line.c_str());
      continue;
    }

    auto it = anchor_idx.find(id);
    if (it == anchor_idx.end()) {
      ROS_WARN_THROTTLE(10.0, "uwb_node: unknown anchor id '%s' — add to params.yaml",
                        id.c_str());
      continue;
    }
    const size_t idx = it->second;

    // Device timestamp scaled to seconds — used for EKF dt so that prediction
    // reflects the device clock rather than ROS arrival jitter.
    // header.stamp still uses ros::Time::now() until epoch offset is calibrated.
    const double t_device = device_ts_raw * timestamp_scale;
    const double t_now    = ros::Time::now().toSec();

    cache[idx]       = {range, rx_power, true};
    last_ranges[idx] = range;

    // ---- Initialisation via trilateration ----
    if (!ekf.isInitialized()) {
      // Collect valid anchors that have at least one measurement.
      std::vector<uwb_positioning::Anchor> valid_anchors;
      std::vector<double> valid_ranges, valid_weights;
      for (size_t i = 0; i < anchor_list.size(); ++i) {
        if (!cache[i].valid) continue;
        valid_anchors.push_back(anchor_list[i]);
        valid_ranges.push_back(cache[i].range);
        valid_weights.push_back(
            rxToWeight(cache[i].rx_power, rx_ref_dbm, rx_decay_db, sigma_r));
      }
      if (valid_anchors.size() < 3) {
        ROS_INFO_THROTTLE(2.0, "uwb_node: waiting for >= 3 anchors to initialise "
                               "(%zu/%zu)", valid_anchors.size(), anchor_list.size());
        continue;
      }
      double ix, iy, iz;
      if (!uwb_positioning::trilaterate(valid_anchors, valid_ranges, valid_weights,
                                        ix, iy, iz)) {
        ROS_WARN_THROTTLE(2.0, "uwb_node: trilateration failed (degenerate geometry) "
                               "— check anchor positions");
        continue;
      }
      ekf.init(ix, iy, iz, t_device);
      ROS_INFO("uwb_node: EKF initialised at (%.3f, %.3f, %.3f)", ix, iy, iz);
      continue;  // use next frame for first predict+update cycle
    }

    // ---- Predict + sequential update ----
    ekf.predict(t_device);  // dt computed from device clock for accurate timing
    ekf.update(anchor_list[idx], range, rx_power);

    // ---- Publish ----
    uwb_positioning::UwbPosition msg;
    msg.header.stamp    = ros::Time::now();
    msg.header.frame_id = frame_id;
    msg.x = ekf.posX();
    msg.y = ekf.posY();
    msg.z = ekf.posZ();

    // Fill 3x3 position covariance (row-major)
    const Eigen::Matrix3d cov = ekf.positionCovariance();
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        msg.position_covariance[r*3 + c] = cov(r, c);

    msg.anchor_distances = last_ranges;
    pub.publish(msg);

    ros::spinOnce();
  }

  sp.close();
  return 0;
}
