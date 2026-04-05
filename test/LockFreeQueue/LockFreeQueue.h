/**
 * @file LockFreeQueue.h
 * @brief MPMC无锁队列实现，专为void()类型的可调用对象设计
 * 
 * 这是一个多生产者多消费者（MPMC）无锁队列，用于存储和执行函数对象（functor）。
 * 实现基于环形缓冲区，使用原子操作实现线程安全，无需锁。
 * 设计简洁，适合新手学习无锁编程基础。
 * 
 * 队列容量固定为1024，使用环形缓冲区实现，保证多线程安全。
 */

#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <memory>

/**
 * @class LockFreeQueue
 * @brief 无锁队列，支持多生产者多消费者
 * 
 * 队列使用固定大小的环形缓冲区存储std::function<void()>对象。
 * 使用原子索引保证线程安全，通过CAS（Compare-And-Swap）操作实现无锁。
 * 容量固定为1024（2的10次方），使用位运算优化环形索引计算。
 */
class LockFreeQueue {
    // 队列容量，必须是2的幂次方（为了使用位运算优化）
    static constexpr size_t CAPACITY = 1024;
    static_assert((CAPACITY & (CAPACITY - 1)) == 0, "CAPACITY must be a power of 2");
    
private:
    using Task = std::function<void()>;
    
    // 环形缓冲区
    std::unique_ptr<Task[]> buffer;
    
    // 原子索引：head指向下一个要读取的位置，tail指向下一个要写入的位置
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};
    
    // 掩码，用于环形缓冲区的索引计算（因为CAPACITY是2的幂次方）
    static constexpr size_t MASK = CAPACITY - 1;
    
public:
    /**
     * @brief 构造函数，初始化缓冲区
     */
    LockFreeQueue();
    
    /**
     * @brief 析构函数
     */
    ~LockFreeQueue() = default;
    
    /**
     * @brief 尝试将一个任务推入队列
     * 
     * @param task 要添加的可调用对象
     * @return true 如果成功入队
     * @return false 如果队列已满
     */
    bool try_enqueue(Task task);
    
    /**
     * @brief 尝试从队列中取出一个任务
     * 
     * @param task 用于接收取出的任务
     * @return true 如果成功出队
     * @return false 如果队列为空
     */
    bool try_dequeue(Task& task);
    
    /**
     * @brief 检查队列是否为空
     * 
     * @return true 如果队列为空
     */
    bool empty() const;
    
    /**
     * @brief 检查队列是否已满
     * 
     * @return true 如果队列已满
     */
    bool full() const;
    
    /**
     * @brief 获取队列当前大小（近似值，在多线程环境下仅供参考）
     * 
     * @return 队列中的元素数量（近似值）
     */
    size_t size() const;
    
    /**
     * @brief 获取队列容量
     * 
     * @return 队列容量
     */
    constexpr size_t capacity() const { return CAPACITY; }
    
private:
    // 禁用拷贝和赋值
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    
    // 禁用移动
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator=(LockFreeQueue&&) = delete;
};