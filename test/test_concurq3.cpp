#include <cstdio>
#include <functional>

#include "blockingconcurrentqueue.h"

int main() {
  moodycamel::BlockingConcurrentQueue<int> q
  // (513)
  ;

  // std::thread producer([&]() {
  //   for (int i = 0; i != 100; ++i) {
  //     std::this_thread::sleep_for(std::chrono::milliseconds(i % 10));
  //     q.enqueue(i);
  //   }
  // });

  std::thread producer([&]() {
    for (int i = 0; i != 1000; ++i) {
      while (!q.try_enqueue(i)) {
        std::printf("Failed to try enqueue: %d\n", i);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }
  });

  // std::thread consumer([&]() {
  //   for (int i = 0; i != 100; ++i) {
  //     int item;
  //     q.wait_dequeue(item);
  //     assert(item == i);

  //     if (q.wait_dequeue_timed(item, std::chrono::milliseconds(5))) {
  //       ++i;
  //       assert(item == i);
  //     }
  //   }
  // });

  // std::thread consumer([&]() {
  //   for (int i = 0; i != 300; ++i) {
  //     int item;
  //     auto success = q.wait_dequeue_timed(item,
  //     std::chrono::milliseconds(5)); if (success) {
  //       std::printf("i: %d, item: %d\n", i, item);
  //     }
  //   }
  // });

  producer.join();
  // consumer.join();

  assert(q.size_approx() == 0);
}
