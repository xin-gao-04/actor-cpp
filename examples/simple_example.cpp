#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <any>  // C++17标准库
#include <map>

#include "actor.h"
#include "event_loop.h"
#include "message.h"
#include "scheduler.h"

// 自定义Actor类
class PingActor : public Actor {
public:
    PingActor(const std::string& name, std::weak_ptr<EventLoop> event_loop)
        : Actor(name, event_loop) {
        
        // 在构造函数中不再直接注册处理函数，而是在initialize方法中
    }
    
    // 重写initialize方法
    void initialize() override {
        Actor::initialize();  // 先调用基类方法
        
        // 注册消息处理函数
        register_handler("ping", [this](const Message& msg) {
            handle_ping(msg);
        });
        
        register_handler("pong", [this](const Message& msg) {
            handle_pong(msg);
        });
        
        register_handler("high_priority", [this](const Message& msg) {
            handle_high_priority(msg);
        });
        
        std::cout << "PingActor " << get_name() << " initialized" << std::endl;
    }
    
    // 重写状态变化回调
    void on_state_changed(State old_state, State new_state) override {
        std::cout << "PingActor " << get_name() << " state changed from " 
                  << static_cast<int>(old_state) << " to " 
                  << static_cast<int>(new_state) << std::endl;
    }

private:
    void handle_ping(const Message& msg) {
        std::cout << name_ << " received ping from " << msg.get_sender_id() << std::endl;
        
        // 回复pong消息
        std::map<std::string, std::any> payload;
        payload["count"] = msg.get_payload_value<int>("count") + 1;
        
        Message response("pong", id_, msg.get_sender_id(), payload);
        send(msg.get_sender_id(), response);
    }
    
    void handle_pong(const Message& msg) {
        int count = msg.get_payload_value<int>("count");
        std::cout << name_ << " received pong #" << count << " from " << msg.get_sender_id() << std::endl;
        
        // 如果计数小于10，继续ping-pong
        if (count < 10) {
            std::map<std::string, std::any> payload;
            payload["count"] = count;
            
            // 每隔一次发送一条高优先级消息
            if (count % 2 == 0) {
                Message high_priority_msg("high_priority", id_, msg.get_sender_id(), payload, 
                                          Message::Priority::HIGH);
                send(msg.get_sender_id(), high_priority_msg);
            } else {
                Message ping_msg("ping", id_, msg.get_sender_id(), payload);
                send(msg.get_sender_id(), ping_msg);
            }
        }
    }
    
    void handle_high_priority(const Message& msg) {
        int count = msg.get_payload_value<int>("count");
        std::cout << name_ << " received HIGH PRIORITY message #" << count 
                  << " from " << msg.get_sender_id() << std::endl;
        
        // 回复ping消息
        std::map<std::string, std::any> payload;
        payload["count"] = count;
        
        Message ping_msg("ping", id_, msg.get_sender_id(), payload);
        send(msg.get_sender_id(), ping_msg);
    }
};

int main() {
    // 创建事件循环
    auto event_loop = std::make_shared<EventLoop>();
    
    // 创建两个PingActor
    auto actor1 = std::make_shared<PingActor>("Actor1", event_loop);
    auto actor2 = std::make_shared<PingActor>("Actor2", event_loop);
    
    // 注册actor到事件循环
    event_loop->register_actor(actor1);
    event_loop->register_actor(actor2);
    
    // 选择调度器 - 可以在这里切换不同调度策略测试效果
    // auto scheduler = std::make_shared<RoundRobinScheduler>();
    // auto scheduler = std::make_shared<PriorityScheduler>();
    auto scheduler = std::make_shared<MessagePriorityScheduler>();
    // auto scheduler = std::make_shared<FairScheduler>();
    
    event_loop->set_scheduler(scheduler);
    
    // 初始化Actor（在运行前）- 事件循环启动后也会自动初始化和启动未初始化的Actor
    actor1->initialize();
    actor1->start();
    actor2->initialize();
    actor2->start();
    
    // 启动ping-pong通信
    std::map<std::string, std::any> payload;
    payload["count"] = 1;
    
    Message initial_msg("ping", actor1->get_id(), actor2->get_id(), payload);
    event_loop->deliver_message(initial_msg);
    
    // 运行事件循环（在单独的线程中）
    std::thread event_thread([&event_loop]() {
        event_loop->run();
    });
    
    // 等待一段时间后停止
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 停止Actor和事件循环
    actor1->stop();
    actor2->stop();
    event_loop->stop();
    
    // 等待事件循环线程结束
    if (event_thread.joinable()) {
        event_thread.join();
    }
    
    return 0;
} 