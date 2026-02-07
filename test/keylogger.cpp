#include <linux/input.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

const auto DEV_KBD{"/dev/input/event3"};

int main() {
  std::printf("Key logger active...\n");

  int fd = open(DEV_KBD, O_RDONLY);

  input_event ie;
  for (;;) {
    read(fd, &ie, sizeof(ie));
    std::cout << "time: " << ie.time.tv_sec << ' ';
    std::cout << "type: " << ie.type << ' ';
    std::cout << "code: " << ie.code << '\n';
    std::cout << "value: " << ie.value << ' ';
  }

  return 0;
}

/*
9
*/

// // 检查设备是否为键盘
// int is_keyboard_device(const char* device_path) {
//   int fd = open(device_path, O_RDONLY);
//   if (fd == -1) return 0;

//   unsigned long evbit = 0;
//   ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);

//   // 检查是否支持 EV_KEY 事件类型
//   if (!(evbit & (1 << EV_KEY))) {
//     close(fd);
//     return 0;
//   }

//   // 获取支持的按键位图
//   unsigned char keybit[KEY_MAX / 8 + 1];
//   memset(keybit, 0, sizeof(keybit));
//   ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);

//   // 检查是否有常见的键盘按键
//   int has_keys = 0;
//   for (int i = KEY_A; i <= KEY_Z; i++) {
//     if (keybit[i / 8] & (1 << (i % 8))) {
//       has_keys = 1;
//       break;
//     }
//   }

//   close(fd);
//   return has_keys;
// }
