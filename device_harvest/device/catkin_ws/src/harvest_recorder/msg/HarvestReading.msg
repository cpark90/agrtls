# Combined, time-aligned snapshot of one UWB position + one scale weight.
# header.stamp is the UWB stamp; the scale stamp is preserved inside weight.header.
std_msgs/Header header

uwb_positioning/UwbPosition uwb
scale_reader/WeightReading  weight
