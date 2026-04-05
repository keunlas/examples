/**
 * @file test_simple.cpp
 * @brief 简单测试LockFreeQueue
 */

#include "LockFreeQueue.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

int main() {
    std::cout << "=== Simple LockFreeQueue Test ===" << std::endl;
    
    // 测试1：基本入队出队
    {
        LockFreeQueue queue;
        bool success = false;
        
        // 入队一个任务
        success = queue.try_enqueue([]() {
            std::cout << "Task executed!" << std::endl;
        });
        std::cout << "Test 1.1: Enqueue - " << (success ? "PASS" : "FAIL") << std::endl;
        
        // 出队并执行
        std::function<void()> task;
        success = queue.try_dequeue(task);
        std::cout << "Test 1.2: Dequeue - " << (success ? "PASS" : "FAIL") << std::endl;
        
        if (success && task) {
            std::cout << "  Executing: ";
            task();
        }
        
        // 队列应为空
        std::cout << "Test 1.3: Queue empty - " << (queue.empty() ? "PASS" : "FAIL") << std::endl;
    }
    
    std::cout << "\n=== Multi-threaded Simple Test ===" << std::endl;
    
    // 测试2：单生产者单消费者
    {
        LockFreeQueue queue;
        std::atomic<int> counter{0};
        const int NUM_TASKS = 10000;
        
        // 生产者线程
        auto producer = [&queue, NUM_TASKS]() {
            for (int i = 0; i < NUM_TASKS; ++i) {
                while (!queue.try_enqueue([i]() {
                    // 空任务，仅用于测试
                })) {
                    std::this_thread::yield();
                }
            }
        };
        
        // 消费者线程
        auto consumer = [&queue, &counter, NUM_TASKS]() {
            std::function<void()> task;
            while (counter < NUM_TASKS) {
                if (queue.try_dequeue(task)) {
                    if (task) {
                        task();
                    }
                    ++counter;
                } else {
                    std::this_thread::yield();
                }
            }
        };
        
        std::thread p(producer);
        std::thread c(consumer);
        
        p.join();
        c.join();
        
        std::cout << "Test 2: Single producer, single consumer - ";
        std::cout << (counter == NUM_TASKS ? "PASS" : "FAIL") << std::endl;
        std::cout << "  Counter: " << counter.load() << "/" << NUM_TASKS << std::endl;
        std::cout << "  Queue empty: " << (queue.empty() ? "YES" : "NO") << std::endl;
    }
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    return 0;
}