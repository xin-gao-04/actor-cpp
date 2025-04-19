#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <queue>
#include <functional>
#include <atomic>

class Actor;
class Message;
class Scheduler;

/**
 * @brief EventLoop类 - Actor消息调度的核心
 *
 * 事件循环负责：
 * 1. 管理所有Actor实例
 * 2. 调度消息的传递和处理
 * 3. 维护Actor系统的生命周期
 */
class EventLoop : public std::enable_shared_from_this<EventLoop>
{
public:
    EventLoop();
    ~EventLoop() = default;

    // 运行事件循环直到没有更多消息或被停止
    void run();

    // 停止事件循环
    void stop();

    // 注册Actor到事件循环
    void register_actor(std::shared_ptr<Actor> actor);

    // 从事件循环移除Actor
    void remove_actor(const std::string &actor_id);

    // 查找Actor
    std::shared_ptr<Actor> find_actor(const std::string &actor_id);

    // 传递消息到目标Actor
    void deliver_message(const Message &message);

    // 设置调度器
    void set_scheduler(std::shared_ptr<Scheduler> scheduler);

    // 检查是否还有更多工作要做
    bool has_work() const;

    // 获取当前是否正在运行
    bool is_running() const { return running_; }

private:
    // Actor注册表，通过ID映射
    std::unordered_map<std::string, std::shared_ptr<Actor>> actors_;

    // 事件循环是否正在运行
    std::atomic<bool> running_;

    // 调度器
    std::shared_ptr<Scheduler> scheduler_;

    // 处理一个调度周期
    void process_one_cycle();
};