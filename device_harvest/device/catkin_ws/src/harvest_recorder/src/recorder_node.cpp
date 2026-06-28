// Harvest recorder node.
//
// Subscribes to /uwb/position (UwbPosition) and /scale/weight (WeightReading),
// time-aligns them with ApproximateTime, writes each matched pair to CSV,
// and optionally republishes a combined HarvestReading.
#include <ros/ros.h>
#include <harvest_recorder/HarvestReading.h>
#include <uwb_positioning/UwbPosition.h>
#include <scale_reader/WeightReading.h>

#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using uwb_positioning::UwbPosition;
using scale_reader::WeightReading;
typedef message_filters::sync_policies::ApproximateTime<UwbPosition, WeightReading>
    SyncPolicy;

class HarvestRecorder {
 public:
  HarvestRecorder(ros::NodeHandle& nh, ros::NodeHandle& pnh) {
    std::string uwb_topic, scale_topic, csv_dir, synced_topic;
    double slop = 0.1;
    int queue_size = 50;

    pnh.param<std::string>("uwb_topic",   uwb_topic,   "/uwb/position");
    pnh.param<std::string>("scale_topic", scale_topic, "/scale/weight");
    pnh.param("slop",       slop,       0.1);
    pnh.param("queue_size", queue_size, 50);
    pnh.param<std::string>("csv_path",    csv_dir,     "/data");
    pnh.param("publish_synced", publish_synced_, true);
    pnh.param<std::string>("synced_topic", synced_topic, "/synced/reading");

    if (publish_synced_)
      pub_ = nh.advertise<harvest_recorder::HarvestReading>(synced_topic, 100);

    mkdir(csv_dir.c_str(), 0775);
    const std::string path = csv_dir + "/harvest_" + timestamp() + ".csv";
    csv_.open(path.c_str());
    if (!csv_.is_open()) {
      ROS_FATAL("cannot open CSV: %s", path.c_str());
      throw std::runtime_error("csv open failed");
    }
    csv_ << "t_uwb,t_scale,dt,x,y,z,anchor_distances,weight,unit\n";
    ROS_INFO("recording %s + %s (slop=%.3fs) -> %s",
             uwb_topic.c_str(), scale_topic.c_str(), slop, path.c_str());

    sub_uwb_.subscribe(nh, uwb_topic, 100);
    sub_scale_.subscribe(nh, scale_topic, 100);
    sync_.reset(new message_filters::Synchronizer<SyncPolicy>(
        SyncPolicy(queue_size), sub_uwb_, sub_scale_));
    sync_->setMaxIntervalDuration(ros::Duration(slop));
    sync_->registerCallback(boost::bind(&HarvestRecorder::onPair, this, _1, _2));
  }

  ~HarvestRecorder() { if (csv_.is_open()) csv_.close(); }

 private:
  void onPair(const UwbPosition::ConstPtr& uwb,
              const WeightReading::ConstPtr& scale) {
    const double t_uwb   = uwb->header.stamp.toSec();
    const double t_scale = scale->header.stamp.toSec();
    csv_ << std::fixed << std::setprecision(6)
         << t_uwb << ',' << t_scale << ',' << (t_uwb - t_scale) << ','
         << uwb->x << ',' << uwb->y << ',' << uwb->z << ','
         << joinDoubles(uwb->anchor_distances) << ','
         << scale->weight << ',' << scale->unit << '\n';

    if (publish_synced_) {
      harvest_recorder::HarvestReading msg;
      msg.header.stamp    = uwb->header.stamp;
      msg.header.frame_id = uwb->header.frame_id;
      msg.uwb    = *uwb;
      msg.weight = *scale;
      pub_.publish(msg);
    }
  }

  static std::string joinDoubles(const std::vector<double>& v) {
    std::ostringstream os;
    for (size_t i = 0; i < v.size(); ++i) {
      if (i) os << ' ';
      os << v[i];
    }
    return os.str();
  }

  static std::string timestamp() {
    const std::time_t t = std::time(NULL);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&t));
    return std::string(buf);
  }

  bool publish_synced_ = true;
  ros::Publisher pub_;
  std::ofstream csv_;
  message_filters::Subscriber<UwbPosition>    sub_uwb_;
  message_filters::Subscriber<WeightReading>  sub_scale_;
  boost::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync_;
};

int main(int argc, char** argv) {
  ros::init(argc, argv, "recorder");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");
  HarvestRecorder rec(nh, pnh);
  ros::spin();
  return 0;
}
