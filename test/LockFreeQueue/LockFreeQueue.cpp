/**
 * @file LockFreeQueue.cpp
 * @brief LockFreeQueue类的实现
 * 
 * 实现MPMC无锁队列的核心逻辑，包括任务入队、出队和状态检查。
 * 使用原子操作实现线程安全，无需锁。
 */

#include "LockFreeQueue.h"
#include <atomic>
#include <thread>

/**
 * @brief 构造函数，初始化环形缓冲区
 * 
 * 使用new分配指定容量的缓冲区，并用std::unique_ptr管理。
 * 初始化head和tail为0，表示队列为空。
 */
LockFreeQueue::LockFreeQueue() 
    : buffer(new Task[CAPACITY]) {
    // 构造函数体，head和tail已在成员初始化列表中初始化为0
}

/**
 * @brief 尝试将一个任务推入队列
 * 
 * 使用CAS（Compare-And-Swap）操作实现无锁入队。
 * 多生产者可以同时调用此方法而不会产生数据竞争。
 * 
 * 使用顺序一致内存顺序，简化实现并确保正确性。
 * 
 * @param task 要添加的可调用对象
 * @return true 如果成功入队
 * @return false 如果队列已满
 */
bool LockFreeQueue::try_enqueue(Task task) {
    // 使用顺序一致内存顺序
    size_t current_tail = tail.load(std::memory_order_seq_cst);
    size_t next_tail = (current_tail + 1) & MASK;
    
    // 检查队列是否已满
    size_t current_head = head.load(std::memory_order_seq_cst);
    if (next_tail == current_head) {
        return false; // 队列已满
    }
    
    // 先写入缓冲区（在原子更新之前）
    buffer[current_tail] = std::move(task);
    
    // 然后更新tail，使用顺序一致内存顺序
    tail.store(next_tail, std::memory_order_seq_cst);
    return true;
}

/**
 * @brief 尝试从队列中取出一个任务
 * 
 * 使用CAS（Compare-And-Swap）操作实现无锁出队。
 * 多消费者可以同时调用此方法而不会产生数据竞争。
 * 
 * @param task 用于接收取出的任务
 * @return true 如果成功出队
 * @return false 如果队列为空
 */
bool LockFreeQueue::try_dequeue(Task& task) {
    // 加载当前head
    size_t current_head = head.load(std::memory_order_relaxed);
    
    // 检查队列是否为空
    size_t current_tail = tail.load(std::memory_order_acquire);
    if (current_head == current_tail) {
        return false; // 队列为空
    }
    
    // 计算下一个读取位置
    size_t next_head = (current_head + 1) & MASK;
    
    // 尝试原子地更新head
    if (head.compare_exchange_weak(current_head, next_head,
                                   std::memory_order_acquire,
                                   std::memory_order_relaxed)) {
        // CAS成功，从缓冲区移动任务到输出参数
        task = std::move(buffer[current_head]);
        // 清空缓冲区中的原任务，防止重复执行
        buffer[current_head] = Task{};
        return true;
    }
    
    // CAS失败（其他线程同时修改了head），返回false让调用者重试
    return false;
}

/**
 * @brief 检查队列是否为空
 * 
 * 注意：在多线程环境下，此函数返回的结果只是瞬间状态，
 * 可能在返回后立即被其他线程改变。
 * 
 * @return true 如果队列为空
 */
bool LockFreeQueue::empty() const {
    // 使用memory_order_acquire确保读取到最新的tail值
    return head.load(std::memory_order_acquire) == 
           tail.load(std::memory_order_acquire);
}

/**
 * @brief 检查队列是否已满
 * 
 * 注意：在多线程环境下，此函数返回的结果只是瞬间状态，
 * 可能在返回后立即被其他线程改变。
 * 
 * @return true 如果队列已满
 */
bool LockFreeQueue::full() const {
    // 使用memory_order_acquire确保读取到最新的head和tail值
    size_t current_tail = tail.load(std::memory_order_acquire);
    size_t next_tail = (current_tail + 1) & MASK;
    return next_tail == head.load(std::memory_order_acquire);
}

/**
 * @brief 获取队列当前大小（近似值）
 * 
 * 注意：在多线程环境下，此函数返回的是近似值，
 * 因为head和tail可能在计算过程中被其他线程修改。
 * 此值仅用于监控和调试，不应作为精确计数使用。
 * 
 * @return 队列中的元素数量（近似值）
 */
size_t LockFreeQueue::size() const {
    // 使用memory_order_acquire确保读取到相对一致的值
    size_t current_head = head.load(std::memory_order_acquire);
    size_t current_tail = tail.load(std::memory_order_acquire);
    
    // 计算环形缓冲区中的元素数量
    if (current_tail >= current_head) {
        return current_tail - current_head;
    } else {
        return (CAPACITY - current_head) + current_tail;
    }
}