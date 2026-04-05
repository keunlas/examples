#include <cstdio>

#include "concurrentqueue.h"

#define N 512

int main() {
  moodycamel::ConcurrentQueue<int> q(512);
  std::atomic_bool done{false};

  std::thread producer([&]() {
    for (int i = 1; i <= N; ++i) {
      // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      q.enqueue(i);
    }
  });

  std::thread consumer([&]() {
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    uint64_t cnt = 1;
    while (!done || q.size_approx() > 0) {
      int item;
      bool found = q.try_dequeue(item);
      if (found) {
        std::printf("count: %ld, item: %d, approx_size: %ld\n", cnt, item,
                    q.size_approx());
      }
      cnt += 1;
    }
  });

  producer.join();
  done.store(true);
  consumer.join();
}
