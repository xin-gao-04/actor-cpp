#pragma once

#include <memory>
#include <string>
#include <queue>
#include <unordered_map>
#include <functional>
#include <atomic>
#include "message.h"

class EventLoop;

/**
 * @brief Actor类 - Actor模型的基本单元
 *
 * Actor是一个独立的计算单元，它可以：
 * 1. 接收和处理消息
 * 2. 创建新的Actor
 * 3. 向其他Actor发送消息
 * 4. 更新自己的内部状态
 */
class Actor : public std::enable_shared_from_this<Actor>
{
public:
    using ActorPtr = std::shared_ptr<Actor>;
    using MessageHandler = std::function<void(const Message &)>;

    // Actor生命周期状态
    enum class State
    {
        CREATED,     // 已创建但尚未初始化
        INITIALIZED, // 已初始化
        RUNNING,     // 正在运行
        STOPPING,    // 正在停止
        STOPPED      // 已停止
    };

    explicit Actor(std::string name, std::weak_ptr<EventLoop> event_loop);
    virtual ~Actor() = default;

    // 初始化Actor（设置初始状态和注册基本消息处理器）
    virtual void initialize();

    // 启动Actor（开始处理消息）
    virtual void start();

    // 停止Actor（停止处理新消息，处理完剩余消息后退出）
    virtual void stop();

    // 立即停止Actor（丢弃所有未处理消息）
    virtual void stop_immediately();

    // 接收消息（将消息放入队列）
    void receive(Message message);

    // 处理队列中的下一条消息
    bool process_next_message();

    // 注册消息处理函数
    void register_handler(const std::string &message_type, MessageHandler handler);

    // 向另一个Actor发送消息
    void send(const std::string &target_actor_id, Message message);

    // 创建一个子Actor
    ActorPtr create_child(const std::string &name);

    // 获取Actor的ID
    const std::string &get_id() const { return id_; }

    // 获取Actor的名称
    const std::string &get_name() const { return name_; }

    // 检查消息队列是否为空
    bool has_messages() const { return !message_queue_.empty(); }

    // 获取当前状态
    State get_state() const { return state_; }

    // 判断是否在运行状态
    bool is_running() const { return state_ == State::RUNNING; }

    // 获取消息队列中的消息数量
    size_t message_count() const { return message_queue_.size(); }

    // 查看消息队列中的下一条消息（不会移除）
    Message peek_next_message() const;

    // 获取队列中优先级最高的消息（不会移除）
    Message peek_highest_priority_message() const;

protected:
    // 在状态变化时调用
    virtual void on_state_changed(State old_state, State new_state) {}

    // 设置状态
    void set_state(State new_state);

    // Actor的唯一标识符
    std::string id_;

    // Actor的名称（可读性更好的标识）
    std::string name_;

    // 当前状态
    std::atomic<State> state_;

    // 消息队列
    std::queue<Message> message_queue_;

    // 消息处理函数映射
    std::unordered_map<std::string, MessageHandler> handlers_;

    // 事件循环的弱引用（避免循环引用）
    std::weak_ptr<EventLoop> event_loop_;

    // 生成唯一ID的静态方法
    static std::string generate_id();
};