// Serial frame parser for the UWB32 module.
//
// Frame format (one per line, newline-terminated):
//   "<device_id>,<range_m>,<rx_power_dbm>,<timestamp>"
//   e.g.  "A0,1.234,-78.5,1023456"
//
// device_timestamp is the raw counter value from the device.
// Convert to seconds with: t_s = device_timestamp * timestamp_scale (from params.yaml).
#pragma once

#include <sstream>
#include <string>

namespace uwb_positioning {

struct UwbFrame {
  std::string anchor_id;
  double range_m          = 0.0;
  double rx_power_dbm     = 0.0;
  double device_timestamp = 0.0;  // raw; unit depends on firmware (ms, µs, ...)
};

// Returns false for malformed lines or non-positive range values.
inline bool parseFrame(const std::string& line, UwbFrame& f) {
  std::istringstream ss(line);
  std::string tok;
  if (!std::getline(ss, tok, ',') || tok.empty()) return false;
  f.anchor_id = tok;
  if (!std::getline(ss, tok, ',')) return false;
  try { f.range_m = std::stod(tok); } catch (...) { return false; }
  if (!std::getline(ss, tok, ',')) return false;
  try { f.rx_power_dbm = std::stod(tok); } catch (...) { return false; }
  if (!std::getline(ss, tok, ',')) return false;
  try { f.device_timestamp = std::stod(tok); } catch (...) { return false; }
  return f.range_m > 0.0;
}

}  // namespace uwb_positioning
