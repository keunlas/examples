#include <libudev.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>

// 检测当前已连接的设备
void enumerate_existing_devices(void) {
  struct udev* udev;
  struct udev_enumerate* enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device* dev;

  // 创建udev上下文
  udev = udev_new();
  if (!udev) {
    fprintf(stderr, "无法创建udev上下文\n");
    return;
  }

  // 创建枚举器
  enumerate = udev_enumerate_new(udev);

  // 过滤input子系统设备
  udev_enumerate_add_match_subsystem(enumerate, "input");

  // 扫描设备
  udev_enumerate_scan_devices(enumerate);

  // 获取设备列表
  devices = udev_enumerate_get_list_entry(enumerate);

  printf("=== 当前连接的输入设备 ===\n");

  // 遍历设备列表
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char* path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev, path);

    if (dev) {
      const char* devnode = udev_device_get_devnode(dev);
      const char* product = udev_device_get_property_value(dev, "ID_MODEL");
      const char* vendor = udev_device_get_property_value(dev, "ID_VENDOR");

      // 检查设备类型
      const char* is_keyboard =
          udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");
      const char* is_mouse =
          udev_device_get_property_value(dev, "ID_INPUT_MOUSE");
      const char* is_touchpad =
          udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD");

      if (is_keyboard && strcmp(is_keyboard, "1") == 0) {
        printf("键盘: %s (%s %s)\n", devnode ? devnode : "未知设备节点",
               vendor ? vendor : "未知厂商", product ? product : "未知型号");
      } else if ((is_mouse && strcmp(is_mouse, "1") == 0) ||
                 (is_touchpad && strcmp(is_touchpad, "1") == 0)) {
        const char* type = is_touchpad ? "触摸板" : "鼠标";
        printf("%s: %s (%s %s)\n", type, devnode ? devnode : "未知设备节点",
               vendor ? vendor : "未知厂商", product ? product : "未知型号");
      }

      udev_device_unref(dev);
    }
  }

  // 清理资源
  udev_enumerate_unref(enumerate);
  udev_unref(udev);
}

// 监控设备热插拔事件
void monitor_hotplug_events(void) {
  struct udev* udev;
  struct udev_monitor* mon;
  int fd;

  udev = udev_new();
  if (!udev) {
    fprintf(stderr, "无法创建udev上下文\n");
    return;
  }

  // 创建监控器，监听内核事件
  mon = udev_monitor_new_from_netlink(udev, "udev");
  if (!mon) {
    fprintf(stderr, "无法创建监控器\n");
    udev_unref(udev);
    return;
  }

  // 过滤input子系统事件
  udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);

  // 启用监控
  if (udev_monitor_enable_receiving(mon) < 0) {
    fprintf(stderr, "无法启用监控\n");
    udev_monitor_unref(mon);
    udev_unref(udev);
    return;
  }

  // 获取文件描述符用于select/poll
  fd = udev_monitor_get_fd(mon);

  printf("\n=== 开始监控设备热插拔事件 (Ctrl+C退出) ===\n");

  while (1) {
    fd_set fds;
    struct timeval tv;
    int ret;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    // 设置超时
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    ret = select(fd + 1, &fds, NULL, NULL, &tv);

    if (ret > 0 && FD_ISSET(fd, &fds)) {
      struct udev_device* dev = udev_monitor_receive_device(mon);
      if (dev) {
        const char* action = udev_device_get_action(dev);
        const char* devnode = udev_device_get_devnode(dev);
        const char* product = udev_device_get_property_value(dev, "ID_MODEL");

        // 检查设备类型
        const char* is_keyboard =
            udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");
        const char* is_mouse =
            udev_device_get_property_value(dev, "ID_INPUT_MOUSE");

        if (is_keyboard && strcmp(is_keyboard, "1") == 0) {
          printf("[键盘] %s: %s (%s)\n", action, devnode ? devnode : "未知设备",
                 product ? product : "未知型号");
        } else if (is_mouse && strcmp(is_mouse, "1") == 0) {
          printf("[鼠标] %s: %s (%s)\n", action, devnode ? devnode : "未知设备",
                 product ? product : "未知型号");
        }

        udev_device_unref(dev);
      }
    }
  }

  udev_monitor_unref(mon);
  udev_unref(udev);
}

int main(int argc, char* argv[]) {
  setlocale(LC_ALL, "");

  printf("输入设备检测程序\n");
  printf("================\n");

  // 检测当前已连接的设备
  enumerate_existing_devices();

  // 监控热插拔事件（可选）
  if (argc > 1 && strcmp(argv[1], "--monitor") == 0) {
    monitor_hotplug_events();
  } else {
    printf("\n使用 --monitor 参数启动以监控热插拔事件\n");
  }

  return 0;
}
