#pragma once

#include <string>
#include <map>

#include <libudev.h>

class UDevInterface {
public:
  typedef std::vector<std::pair<std::string, std::string> > DeviceList;

  UDevInterface(bool &reset_flag);
  ~UDevInterface();

  void monitor_thread();

  // This is not thread safe!
  const DeviceList devices() const {
    return m_devices;
  }

  void shutdown() {
    m_run = false;
  }

private:
  struct udev *m_udev;
  DeviceList m_devices;
  std::map<std::pair<std::string, std::string>, std::string> m_driver_map;
  bool &m_reset_flag;
  bool m_run;

  void update_devices();
  std::string card_type(udev_device *dev);
};
