/**
 * @file main.cpp
 * @brief LockFreeQueue的测试程序
 * 
 * 测试MPMC无锁队列的功能和性能，包括：
 * 1. 基本功能测试（单线程）
 * 2. 多生产者多消费者压力测试（百万元素）
 * 3. 正确性验证（所有任务都被执行）
 * 
 * 测试使用多个生产者线程生产打印数字的functor，
 * 多个消费者线程执行这些functor。
 */

#include "LockFreeQueue.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <sstream>
#include <mutex>

// 测试配置
constexpr size_t TOTAL_TASKS = 1000000;    // 总任务数：100万
constexpr size_t NUM_PRODUCERS = 4;        // 生产者线程数
constexpr size_t NUM_CONSUMERS = 4;        // 消费者线程数
constexpr size_t MAX_RETRY_ATTEMPTS = 100; // 入队/出队失败时的最大重试次数

// 全局原子计数器，用于跟踪生产和消费的任务数
std::atomic<size_t> produced_count{0};
std::atomic<size_t> consumed_count{0};
std::atomic<size_t> failed_enqueue_count{0};
std::atomic<size_t> failed_dequeue_count{0};

// 用于同步输出的互斥锁（避免多线程输出混乱）
std::mutex cout_mutex;

/**
 * @brief 生产者线程函数
 * 
 * 每个生产者线程不断创建任务并尝试入队，直到生产足够数量的任务。
 * 任务是一个打印任务ID和生产者ID的lambda函数。
 * 
 * @param queue 无锁队列引用
 * @param producer_id 生产者ID（用于标识）
 */
void producer_thread(LockFreeQueue& queue, int producer_id) {
    while (produced_count < TOTAL_TASKS) {
        // 原子地获取下一个任务ID（使用fetch_add）
        size_t task_id = produced_count.fetch_add(1, std::memory_order_relaxed);
        
        // 如果已经超过总任务数，回退并退出
        if (task_id >= TOTAL_TASKS) {
            produced_count.fetch_sub(1, std::memory_order_relaxed);
            break;
        }
        
        // 创建任务：打印任务ID和生产者ID
        auto task = [task_id, producer_id]() {
            // 在实际应用中，这里可能是实际的工作负载
            // 为了测试，我们只是模拟工作
            std::stringstream ss;
            ss << "Task " << task_id << " produced by P" << producer_id;
            // 为了避免输出影响性能，我们只在特定条件下打印
            if (task_id % 250000 == 0) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << ss.str() << std::endl;
            }
        };
        
        // 尝试入队，如果失败则重试有限次数
        bool success = false;
        for (size_t attempt = 0; attempt < MAX_RETRY_ATTEMPTS; ++attempt) {
            if (queue.try_enqueue(std::move(task))) {
                success = true;
                break;
            }
            // 短暂休眠，避免忙等待
            std::this_thread::yield();
        }
        
        if (!success) {
            failed_enqueue_count.fetch_add(1, std::memory_order_relaxed);
            // 如果重试多次仍然失败，将任务ID退回
            produced_count.fetch_sub(1, std::memory_order_relaxed);
        }
        
        // 避免生产者过快导致队列过载
        if (queue.full()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Producer " << producer_id << " finished." << std::endl;
    }
}

/**
 * @brief 消费者线程函数
 * 
 * 每个消费者线程不断从队列中取出任务并执行，直到所有任务都被消费。
 * 使用全局计数器判断何时停止。
 * 
 * @param queue 无锁队列引用
 * @param consumer_id 消费者ID（用于标识）
 */
void consumer_thread(LockFreeQueue& queue, int consumer_id) {
    while (consumed_count < TOTAL_TASKS) {
        std::function<void()> task;
        
        // 尝试出队，如果失败则重试有限次数
        bool success = false;
        for (size_t attempt = 0; attempt < MAX_RETRY_ATTEMPTS; ++attempt) {
            if (queue.try_dequeue(task)) {
                success = true;
                break;
            }
            // 如果队列为空且所有任务都已生产，检查是否应该退出
            if (produced_count >= TOTAL_TASKS && queue.empty()) {
                // 检查是否还有未消费的任务
                if (consumed_count >= TOTAL_TASKS) {
                    return;
                }
            }
            // 短暂休眠，避免忙等待
            std::this_thread::yield();
        }
        
        if (success) {
            // 执行任务前检查是否为空
            if (task) {
                task();
                consumed_count.fetch_add(1, std::memory_order_relaxed);
                
                // 打印进度（每25万个任务打印一次）
                size_t consumed = consumed_count.load(std::memory_order_relaxed);
                if (consumed % 250000 == 0) {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Consumed " << consumed << " tasks." << std::endl;
                }
            } else {
                // 任务为空，记录错误但不算作成功消费
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "WARNING: Empty task dequeued!" << std::endl;
                failed_dequeue_count.fetch_add(1, std::memory_order_relaxed);
            }
        } else {
            failed_dequeue_count.fetch_add(1, std::memory_order_relaxed);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Consumer " << consumer_id << " finished." << std::endl;
    }
}

/**
 * @brief 基本功能测试（单线程）
 * 
 * 测试队列的基本功能：入队、出队、空/满检查。
 */
void basic_functionality_test() {
    std::cout << "=== Basic Functionality Test ===" << std::endl;
    
    LockFreeQueue queue;
    
    // 测试1：初始状态应为空
    std::cout << "Test 1: Initial queue should be empty - "
              << (queue.empty() ? "PASS" : "FAIL") << std::endl;
    
    // 测试2：入队一个任务
    bool enqueue_success = queue.try_enqueue([]() {
        std::cout << "  Hello from task!" << std::endl;
    });
    std::cout << "Test 2: Enqueue single task - "
              << (enqueue_success ? "PASS" : "FAIL") << std::endl;
    
    // 测试3：队列不应为空
    std::cout << "Test 3: Queue should not be empty after enqueue - "
              << (!queue.empty() ? "PASS" : "FAIL") << std::endl;
    
    // 测试4：出队并执行任务
    std::function<void()> task;
    bool dequeue_success = queue.try_dequeue(task);
    std::cout << "Test 4: Dequeue task - "
              << (dequeue_success ? "PASS" : "FAIL") << std::endl;
    
    if (dequeue_success) {
        std::cout << "  Executing dequeued task: ";
        task();
    }
    
    // 测试5：队列应再次为空
    std::cout << "Test 5: Queue should be empty after dequeue - "
              << (queue.empty() ? "PASS" : "FAIL") << std::endl;
    
    // 测试6：填充队列直到满
    size_t count = 0;
    while (queue.try_enqueue([]() {})) {
        ++count;
    }
    std::cout << "Test 6: Queue capacity is " << queue.capacity()
              << ", filled " << count << " tasks - "
              << (queue.full() ? "PASS" : "FAIL") << std::endl;
    
    // 测试7：队列满时入队应失败
    bool full_enqueue = queue.try_enqueue([]() {});
    std::cout << "Test 7: Enqueue should fail when queue is full - "
              << (!full_enqueue ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "=== Basic Test Completed ===" << std::endl << std::endl;
}

/**
 * @brief 主函数
 * 
 * 执行基本功能测试和多线程压力测试。
 */
int main() {
    std::cout << "LockFreeQueue MPMC Test Program" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "Total tasks: " << TOTAL_TASKS << std::endl;
    std::cout << "Producers: " << NUM_PRODUCERS << std::endl;
    std::cout << "Consumers: " << NUM_CONSUMERS << std::endl;
    std::cout << "Queue capacity: " << LockFreeQueue{}.capacity() << std::endl;
    std::cout << std::endl;
    
    // 步骤1：基本功能测试
    basic_functionality_test();
    
    // 步骤2：多线程压力测试
    std::cout << "=== Multi-threaded Stress Test ===" << std::endl;
    std::cout << "Starting " << NUM_PRODUCERS << " producers and "
              << NUM_CONSUMERS << " consumers..." << std::endl;
    
    LockFreeQueue queue;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // 重置计数器
    produced_count = 0;
    consumed_count = 0;
    failed_enqueue_count = 0;
    failed_dequeue_count = 0;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建生产者线程
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer_thread, std::ref(queue), i);
    }
    
    // 创建消费者线程
    for (size_t i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer_thread, std::ref(queue), i);
    }
    
    // 等待所有生产者完成
    for (auto& producer : producers) {
        producer.join();
    }
    
    // 等待所有消费者完成
    for (auto& consumer : consumers) {
        consumer.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // 输出测试结果
    std::cout << std::endl;
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Total tasks to produce: " << TOTAL_TASKS << std::endl;
    std::cout << "Actually produced: " << produced_count.load() << std::endl;
    std::cout << "Actually consumed: " << consumed_count.load() << std::endl;
    std::cout << "Failed enqueue attempts: " << failed_enqueue_count.load() << std::endl;
    std::cout << "Failed dequeue attempts: " << failed_dequeue_count.load() << std::endl;
    
    // 验证正确性
    bool all_tasks_consumed = (consumed_count.load() == TOTAL_TASKS);
    bool queue_empty_at_end = queue.empty();
    bool correct_size = (queue.size() == 0);
    
    std::cout << std::endl;
    std::cout << "=== Correctness Verification ===" << std::endl;
    std::cout << "All tasks consumed: " 
              << (all_tasks_consumed ? "PASS" : "FAIL") << std::endl;
    std::cout << "Queue empty at end: " 
              << (queue_empty_at_end ? "PASS" : "FAIL") << std::endl;
    std::cout << "Queue size is 0: " 
              << (correct_size ? "PASS" : "FAIL") << std::endl;
    
    std::cout << std::endl;
    std::cout << "=== Performance Metrics ===" << std::endl;
    std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;
    double tasks_per_second = (TOTAL_TASKS * 1000.0) / duration.count();
    std::cout << "Throughput: " << tasks_per_second << " tasks/second" << std::endl;
    
    if (all_tasks_consumed && queue_empty_at_end) {
        std::cout << std::endl << "SUCCESS: All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << std::endl << "FAILURE: Some tests failed!" << std::endl;
        return 1;
    }
}