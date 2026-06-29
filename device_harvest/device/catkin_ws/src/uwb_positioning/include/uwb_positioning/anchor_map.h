// Anchor data types and ID-to-index lookup table.
#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace uwb_positioning {

struct Anchor {
  std::string id;
  double x = 0.0, y = 0.0, z = 0.0;
};

// Maps device_id strings to Anchor entries and their ordered index.
// The index is stable after construction and matches anchor_distances[] in UwbPosition.msg.
class AnchorMap {
 public:
  void add(const Anchor& a) {
    if (index_.count(a.id))
      throw std::runtime_error("duplicate anchor id: " + a.id);
    index_[a.id] = anchors_.size();
    anchors_.push_back(a);
  }

  // Returns true and sets idx when id is found.
  bool find(const std::string& id, size_t& idx) const {
    auto it = index_.find(id);
    if (it == index_.end()) return false;
    idx = it->second;
    return true;
  }

  const Anchor& at(size_t idx) const { return anchors_.at(idx); }
  size_t size()  const { return anchors_.size(); }
  bool   empty() const { return anchors_.empty(); }
  const std::vector<Anchor>& all() const { return anchors_; }

 private:
  std::vector<Anchor> anchors_;
  std::unordered_map<std::string, size_t> index_;
};

}  // namespace uwb_positioning
