#include <cstdio>
#include <functional>

#include "concurrentqueue.h"

#define N 102400

struct Task {
  int value;
  Task() : Task(0) {}
  Task(int v) : value(v) {}
  inline void operator()() { std::printf("value: %d\n", value); }
};

int main() {
  moodycamel::ConcurrentQueue<std::function<void()>> q(8);
  std::atomic_bool done{false};

  std::thread producer([&]() {
    for (int i = 1; i <= N; ++i) {
      q.enqueue(Task(i));
    }
  });

  std::thread consumer([&]() {
    uint64_t cnt = 10000001;
    while (!done || q.size_approx() > 0) {
      // Task item;
      std::function<void()> item;
      bool found = q.try_dequeue(item);
      if (found) {
        // std::printf("count: %ld, approx_size: %ld, ", cnt, q.size_approx());
        item();
      }
      cnt += 1;
    }
  });
  std::thread consumer2([&]() {
    uint64_t cnt = 20000001;
    while (!done || q.size_approx() > 0) {
      // Task item;
      std::function<void()> item;
      bool found = q.try_dequeue(item);
      if (found) {
        // std::printf("count: %ld, approx_size: %ld, ", cnt, q.size_approx());
        item();
      }
      cnt += 1;
    }
  });
  std::thread consumer3([&]() {
    uint64_t cnt = 30000001;
    while (!done || q.size_approx() > 0) {
      // Task item;
      std::function<void()> item;
      bool found = q.try_dequeue(item);
      if (found) {
        // std::printf("count: %ld, approx_size: %ld, ", cnt, q.size_approx());
        item();
      }
      cnt += 1;
    }
  });

  std::thread producer2([&]() {
    for (int i = 1; i <= N; ++i) {
      q.enqueue(Task(i));
    }
  });

  producer.join();
  producer2.join();
  done.store(true);
  consumer.join();
  consumer2.join();
  consumer3.join();
}
