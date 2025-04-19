#include "actor.h"
#include "event_loop.h"
#include <iostream>
#include <random>
#include <sstream>

Actor::Actor(std::string name, std::weak_ptr<EventLoop> event_loop)
    : name_(std::move(name))
    , event_loop_(std::move(event_loop))
    , state_(State::CREATED) {
    id_ = generate_id();
}

void Actor::initialize() {
    if (state_ != State::CREATED) {
        State current_state = state_.load();
        std::cerr << "Cannot initialize Actor " << name_ << " in state " 
                 << static_cast<int>(current_state) << std::endl;
        return;
    }
    
    set_state(State::INITIALIZED);
}

void Actor::start() {
    if (state_ != State::INITIALIZED) {
        State current_state = state_.load();
        std::cerr << "Cannot start Actor " << name_ << " in state " 
                 << static_cast<int>(current_state) << std::endl;
        return;
    }
    
    set_state(State::RUNNING);
}

void Actor::stop() {
    if (state_ == State::STOPPED || state_ == State::STOPPING) {
        return;  // 已经在停止或已停止
    }
    
    set_state(State::STOPPING);
    
    // 如果消息队列为空，立即停止
    if (!has_messages()) {
        set_state(State::STOPPED);
    }
    // 否则，在process_next_message中处理完所有消息后会停止
}

void Actor::stop_immediately() {
    // 清空消息队列
    std::queue<Message> empty;
    std::swap(message_queue_, empty);
    
    // 设置状态为已停止
    set_state(State::STOPPED);
}

void Actor::set_state(State new_state) {
    State old_state = state_.load();
    state_ = new_state;
    
    // 调用状态变化回调
    on_state_changed(old_state, new_state);
    
    // 记录状态变化
    std::cout << "Actor " << name_ << " (ID: " << id_ << ") state changed: " 
              << static_cast<int>(old_state) << " -> " 
              << static_cast<int>(new_state) << std::endl;
}

void Actor::receive(Message message) {
    // 不在运行状态时拒绝接收新消息
    if (state_ != State::RUNNING && state_ != State::STOPPING) {
        State current_state = state_.load();
        std::cerr << "Actor " << name_ << " rejected message in state " 
                  << static_cast<int>(current_state) << std::endl;
        return;
    }
    
    message_queue_.push(std::move(message));
}

bool Actor::process_next_message() {
    // 停止状态不处理消息
    if (state_ == State::STOPPED) {
        return false;
    }
    
    if (message_queue_.empty()) {
        // 如果状态是STOPPING且消息队列为空，则完成停止过程
        if (state_ == State::STOPPING) {
            set_state(State::STOPPED);
        }
        return false;
    }

    Message message = std::move(message_queue_.front());
    message_queue_.pop();

    auto it = handlers_.find(message.get_type());
    if (it != handlers_.end()) {
        // 找到处理函数，调用它
        it->second(message);
    } else {
        // 没有找到处理函数，打印警告
        std::cerr << "Actor " << name_ << " (ID: " << id_ 
                  << ") received unhandled message type: " 
                  << message.get_type() << std::endl;
    }

    // 如果状态是STOPPING且消息队列为空，则完成停止过程
    if (state_ == State::STOPPING && message_queue_.empty()) {
        set_state(State::STOPPED);
    }
    
    return true;
}

void Actor::register_handler(const std::string& message_type, MessageHandler handler) {
    handlers_[message_type] = std::move(handler);
}

void Actor::send(const std::string& target_actor_id, Message message) {
    auto event_loop = event_loop_.lock();
    if (!event_loop) {
        std::cerr << "Failed to send message: event loop no longer exists" << std::endl;
        return;
    }
    
    // 确保消息的发送者ID和目标ID正确设置
    if (message.get_sender_id().empty()) {
        message = Message(message.get_type(), id_, target_actor_id, message.get_payload());
    }
    
    // 确保目标ID正确
    if (message.get_target_id() != target_actor_id) {
        message = Message(message.get_type(), message.get_sender_id(), 
                         target_actor_id, message.get_payload());
    }

    event_loop->deliver_message(message);
}

Actor::ActorPtr Actor::create_child(const std::string& name) {
    auto event_loop = event_loop_.lock();
    if (!event_loop) {
        std::cerr << "Failed to create child actor: event loop no longer exists" << std::endl;
        return nullptr;
    }

    auto child = std::make_shared<Actor>(name, event_loop_);
    event_loop->register_actor(child);
    return child;
}

Message Actor::peek_next_message() const {
    if (message_queue_.empty()) {
        return Message("empty", "", "", {});
    }
    return message_queue_.front();
}

Message Actor::peek_highest_priority_message() const {
    if (message_queue_.empty()) {
        return Message("empty", "", "", {});
    }
    
    // 创建一个临时队列的副本
    auto queue_copy = message_queue_;
    Message highest_priority_msg = queue_copy.front();
    queue_copy.pop();
    
    // 遍历队列找出最高优先级的消息
    while (!queue_copy.empty()) {
        Message current = queue_copy.front();
        queue_copy.pop();
        
        if (static_cast<int>(current.get_priority()) > 
            static_cast<int>(highest_priority_msg.get_priority())) {
            highest_priority_msg = current;
        }
    }
    
    return highest_priority_msg;
}

std::string Actor::generate_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    
    for (int i = 0; i < 8; ++i) {
        ss << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 4; ++i) {
        ss << dis(gen);
    }
    ss << "-4";  // Version 4 UUID
    for (int i = 0; i < 3; ++i) {
        ss << dis(gen);
    }
    ss << "-";
    ss << static_cast<char>((dis(gen) & 0x3) | 0x8);  // Variant
    for (int i = 0; i < 3; ++i) {
        ss << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 12; ++i) {
        ss << dis(gen);
    }
    
    return ss.str();
} 