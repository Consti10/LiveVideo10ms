#include <unistd.h>
#include <string.h>

#include <thread>
#include <chrono>

#include <logging.hh>
#include <udev_interface.hh>

UDevInterface::UDevInterface(bool &reset_flag) : m_udev(0), m_reset_flag(reset_flag), m_run(true) {

  // Connect to udev
  if ((m_udev = udev_new()) == 0) {
    LOG_ERROR << "Error connecting to udev";
    return;
  }

  // Initialize the list of driver name / model ID to canonical name conversion
  m_driver_map[std::make_pair(std::string("rtl8812au"), std::string(""))] = "rtl8812au";
  m_driver_map[std::make_pair(std::string("rtl88xxau"), std::string(""))] = "rtl8812au";
  m_driver_map[std::make_pair(std::string("88XXau"), std::string(""))] = "rtl8812au";
  m_driver_map[std::make_pair(std::string("rtl88XXau"), std::string(""))] = "rtl8812au";
  m_driver_map[std::make_pair(std::string("ath9k_htc"), std::string("7710"))] = "ar7010";
  m_driver_map[std::make_pair(std::string("ath9k_htc"), std::string("9271"))] = "ar9271";
}

UDevInterface::~UDevInterface() {
  if (m_udev) {
    udev_unref(m_udev);
  }
}


std::string UDevInterface::card_type(udev_device *dev) {
  const char *driver = udev_device_get_property_value(dev, "ID_NET_DRIVER");
  if (!driver) {
    return std::string("");
  }
  const char *model = udev_device_get_property_value(dev, "ID_MODEL_ID");
  std::string dev_model;
  if (model) {
    dev_model = std::string(dev_model);
  }
  auto itr = m_driver_map.find(std::make_pair(std::string(driver), dev_model));
  if (itr != m_driver_map.end()) {
    return itr->second;
  }
  itr = m_driver_map.find(std::make_pair(std::string(driver), std::string("")));
  if (itr != m_driver_map.end()) {
    return itr->second;
  }
  return std::string("");
}

void UDevInterface::update_devices() {
  m_devices.clear();

  // Create the class for enumerating devices.
  struct udev_enumerate *enumerate = udev_enumerate_new(m_udev);
  if (!enumerate) {
    LOG_ERROR << "Error creating the udev enumerator";
    return;
  }

  // Get a list of the network devices
  udev_enumerate_add_match_subsystem(enumerate, "net");
  udev_enumerate_scan_devices(enumerate);
  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  if (!devices) {
    LOG_ERROR << "Error getting the device list";
    udev_enumerate_unref(enumerate);
    return;
  }

  // Iterate over each device
  struct udev_list_entry *dev_list_entry;
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char *path = udev_list_entry_get_name(dev_list_entry);
    udev_device *dev = udev_device_new_from_syspath(m_udev, path);
    const char *type = udev_device_get_devtype(dev);

    // Specifically, we're looking for wlan devices
    if (type && (strncmp(type, "wlan", 4) == 0)) {
      LOG_DEBUG << "I: DEVNOD=" <<  udev_device_get_devnode(dev);
      LOG_DEBUG << "I: DEVNAME=" << udev_device_get_sysname(dev);
      LOG_DEBUG << "I: DEVPATH=" << udev_device_get_devpath(dev);
      LOG_DEBUG << "---";
      std::string dev_name(udev_device_get_sysname(dev));
      std::string driver = card_type(dev);
      if (driver != "") {
        LOG_INFO << "Detected Wifi device " << dev_name << " of type " << driver;
        m_devices.push_back(std::make_pair(dev_name, driver));
      }
    }
    udev_device_unref(dev);
  }
  udev_enumerate_unref(enumerate);
}

void UDevInterface::monitor_thread() {
  LOG_DEBUG << "Monitoring for network device changes";

  // Initialize the list of devices
  update_devices();

  // Create the interface for monitoring USB network device activity
  struct udev_monitor *monitor = udev_monitor_new_from_netlink(m_udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", NULL);
  udev_monitor_enable_receiving(monitor);
  int fd = udev_monitor_get_fd(monitor);

  while (m_run) {

    // Wait for an event on the given file descriptor
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int ret = select(fd+1, &fds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(fd, &fds)) {

      // Get the device that registered the change
      struct udev_device *dev = udev_monitor_receive_device(monitor);
      if (dev) {
        LOG_DEBUG << "I: ACTION=" <<  udev_device_get_action(dev);
        LOG_DEBUG << "I: DEVNAME=" << udev_device_get_sysname(dev);
        LOG_DEBUG << "I: DEVPATH=" << udev_device_get_devpath(dev);
        LOG_DEBUG << "---";
        udev_device_unref(dev);
        // Update the device list with the changes.
        update_devices();
        // Inform other threads that they should update their device list.
        // This should be a signal of some sort...
        m_reset_flag = true;
      }
    }

    // Don't sit in a tight loop
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
