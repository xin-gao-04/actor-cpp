#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <chrono>
#include <queue>

class Actor;
class Message;

/**
 * @brief Scheduler类 - Actor调度策略的抽象
 * 
 * 调度器负责：
 * 1. 决定下一个要处理消息的Actor
 * 2. 实现不同的调度策略（轮询、优先级等）
 */
class Scheduler {
public:
    virtual ~Scheduler() = default;
    
    // 选择下一个要处理消息的Actor
    virtual std::shared_ptr<Actor> next_actor(const std::vector<std::shared_ptr<Actor>>& actors) = 0;
};

/**
 * @brief RoundRobinScheduler类 - 简单的轮询调度器
 * 
 * 轮询调度器按顺序轮流处理每个有消息的Actor
 */
class RoundRobinScheduler : public Scheduler {
public:
    RoundRobinScheduler();
    
    // 实现轮询策略的next_actor方法
    std::shared_ptr<Actor> next_actor(const std::vector<std::shared_ptr<Actor>>& actors) override;

private:
    // 当前Actor索引
    size_t current_index_;
};

/**
 * @brief PriorityScheduler类 - 基于优先级的调度器
 * 
 * 优先级调度器根据Actor的优先级来选择下一个处理消息的Actor
 */
class PriorityScheduler : public Scheduler {
public:
    // 优先级评估函数类型
    using PriorityFunction = std::function<int(const std::shared_ptr<Actor>&)>;
    
    // 构造函数，可以传入自定义的优先级评估函数
    explicit PriorityScheduler(PriorityFunction priority_func = nullptr);
    
    // 实现优先级策略的next_actor方法
    std::shared_ptr<Actor> next_actor(const std::vector<std::shared_ptr<Actor>>& actors) override;

private:
    // 优先级评估函数
    PriorityFunction priority_func_;
    
    // 默认优先级评估函数实现
    int default_priority(const std::shared_ptr<Actor>& actor);
};

/**
 * @brief MessagePriorityScheduler类 - 基于消息优先级的调度器
 * 
 * 此调度器会选择下一条消息优先级最高的Actor进行处理
 */
class MessagePriorityScheduler : public Scheduler {
public:
    MessagePriorityScheduler();
    
    // 基于消息优先级实现next_actor方法
    std::shared_ptr<Actor> next_actor(const std::vector<std::shared_ptr<Actor>>& actors) override;
    
private:
    // 获取Actor当前队列中最高优先级的消息
    Message peek_highest_priority_message(const std::shared_ptr<Actor>& actor);
};

/**
 * @brief FairScheduler类 - 公平调度器
 * 
 * 此调度器确保每个Actor获得相对公平的时间片，
 * 防止某些Actor长时间处于饥饿状态
 */
class FairScheduler : public Scheduler {
public:
    FairScheduler(std::chrono::milliseconds max_starvation_time = std::chrono::seconds(5));
    
    // 根据Actor的等待时间实现公平调度
    std::shared_ptr<Actor> next_actor(const std::vector<std::shared_ptr<Actor>>& actors) override;
    
private:
    // 记录每个Actor上次被调度的时间
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_scheduled_;
    
    // 最大允许的饥饿时间
    std::chrono::milliseconds max_starvation_time_;
}; 