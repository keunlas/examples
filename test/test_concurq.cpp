#include <cstdio>
#include <vector>

#include "concurrentqueue.h"

#define N 4

int main() {
  moodycamel::ConcurrentQueue<int> q1(512);
  auto q = std::move(q1);

  std::atomic_bool done{false};

  std::thread producer([&]() {
    std::vector<int> vec;
    for (int i = 1; i <= N; ++i) {
      // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      vec.push_back(i * 100);
      q.enqueue(i);
    }
    q.enqueue_bulk(vec.begin(), vec.size());
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
