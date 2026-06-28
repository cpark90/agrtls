// Minimal POSIX (termios) serial port wrapper — header-only, no external deps.
// select()-based read timeout keeps the loop responsive to ROS shutdown.
#pragma once

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace scale_reader {

class SerialPort {
 public:
  SerialPort() : fd_(-1) {}
  ~SerialPort() { close(); }

  void open(const std::string& port, int baud) {
    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0)
      throw std::runtime_error("open(" + port + "): " + std::strerror(errno));
    termios tty;
    std::memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd_, &tty) != 0) {
      ::close(fd_); fd_ = -1;
      throw std::runtime_error("tcgetattr: " + std::string(std::strerror(errno)));
    }
    cfmakeraw(&tty);
    const speed_t spd = toSpeed(baud);
    cfsetispeed(&tty, spd);
    cfsetospeed(&tty, spd);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
      ::close(fd_); fd_ = -1;
      throw std::runtime_error("tcsetattr: " + std::string(std::strerror(errno)));
    }
  }

  // Returns true when a newline-terminated line is ready; false on timeout.
  bool readLine(std::string& line, double timeout_s) {
    while (true) {
      const size_t nl = buf_.find('\n');
      if (nl != std::string::npos) {
        line = buf_.substr(0, nl);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        buf_.erase(0, nl + 1);
        return true;
      }
      if (!waitReadable(timeout_s)) return false;
      char chunk[256];
      const ssize_t n = ::read(fd_, chunk, sizeof(chunk));
      if (n > 0) buf_.append(chunk, static_cast<size_t>(n));
    }
  }

  void close() { if (fd_ >= 0) { ::close(fd_); fd_ = -1; } }
  bool isOpen() const { return fd_ >= 0; }

 private:
  bool waitReadable(double timeout_s) {
    fd_set set; FD_ZERO(&set); FD_SET(fd_, &set);
    timeval tv;
    tv.tv_sec  = static_cast<long>(timeout_s);
    tv.tv_usec = static_cast<long>((timeout_s - tv.tv_sec) * 1e6);
    const int r = ::select(fd_ + 1, &set, NULL, NULL, &tv);
    return r > 0 && FD_ISSET(fd_, &set);
  }

  static speed_t toSpeed(int baud) {
    switch (baud) {
      case 9600:   return B9600;
      case 19200:  return B19200;
      case 38400:  return B38400;
      case 57600:  return B57600;
      case 115200: return B115200;
      case 230400: return B230400;
      case 460800: return B460800;
      case 921600: return B921600;
      default: throw std::runtime_error("unsupported baud: " + std::to_string(baud));
    }
  }

  int fd_;
  std::string buf_;
};

}  // namespace scale_reader
