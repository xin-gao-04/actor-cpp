#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <any>

#include "actor.h"
#include "event_loop.h"
#include "message.h"
#include "scheduler.h"

// 测试用的Actor
class TestActor : public Actor
{
public:
    TestActor(const std::string &name, std::weak_ptr<EventLoop> event_loop)
        : Actor(name, event_loop), message_count_(0)
    {

        // 注册测试消息处理函数
        register_handler("test", [this](const Message &msg)
                         { handle_test(msg); });
    }

    int get_message_count() const
    {
        return message_count_;
    }

private:
    std::atomic<int> message_count_;

    void handle_test(const Message &msg)
    {
        message_count_++;

        // 获取发送者信息
        std::string sender_id = msg.get_sender_id();

        // 如果消息中有"reply"标记，就回复一条消息
        if (msg.has_payload_key("reply") && msg.get_payload_value<bool>("reply"))
        {
            Message response("test", id_, sender_id);
            send(sender_id, response);
        }
    }
};

// 测试基本的Actor功能
void test_basic_actor()
{
    std::cout << "Running basic actor test..." << std::endl;

    auto event_loop = std::make_shared<EventLoop>();
    auto actor = std::make_shared<TestActor>("TestActor", event_loop);
    event_loop->register_actor(actor);

    // 确保Actor初始化和启动
    actor->initialize();
    actor->start();

    // 发送测试消息
    Message msg("test", "sender", actor->get_id());
    event_loop->deliver_message(msg);

    // 运行事件循环一小段时间
    std::thread event_thread([&event_loop]()
                             { event_loop->run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    event_loop->stop();

    if (event_thread.joinable())
    {
        event_thread.join();
    }

    // 验证消息已经被处理
    assert(actor->get_message_count() == 1);
    std::cout << "Basic actor test passed!" << std::endl;
}

// 测试Actor之间的通信
void test_actor_communication()
{
    std::cout << "Running actor communication test..." << std::endl;

    auto event_loop = std::make_shared<EventLoop>();
    auto actor1 = std::make_shared<TestActor>("Actor1", event_loop);
    auto actor2 = std::make_shared<TestActor>("Actor2", event_loop);

    event_loop->register_actor(actor1);
    event_loop->register_actor(actor2);

    // 确保两个Actor都初始化和启动
    actor1->initialize();
    actor1->start();
    actor2->initialize();
    actor2->start();

    // 创建一个请求回复的消息
    std::map<std::string, std::any> payload;
    payload["reply"] = true;

    Message msg("test", actor1->get_id(), actor2->get_id(), payload);
    event_loop->deliver_message(msg);

    // 运行事件循环一小段时间
    std::thread event_thread([&event_loop]()
                             { event_loop->run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    event_loop->stop();

    if (event_thread.joinable())
    {
        event_thread.join();
    }

    // 验证两个Actor都处理了消息
    assert(actor2->get_message_count() == 1); // 接收者处理了一条消息
    assert(actor1->get_message_count() == 1); // 发送者接收到了回复

    std::cout << "Actor communication test passed!" << std::endl;
}

// 测试不同的调度器
void test_schedulers()
{
    std::cout << "Running scheduler test..." << std::endl;

    auto event_loop = std::make_shared<EventLoop>();

    // 创建并注册10个actor
    std::vector<std::shared_ptr<TestActor>> actors;
    for (int i = 0; i < 10; ++i)
    {
        auto actor = std::make_shared<TestActor>("Actor" + std::to_string(i), event_loop);
        event_loop->register_actor(actor);
        actors.push_back(actor);

        // 确保Actor初始化和启动
        actor->initialize();
        actor->start();
    }

    // 向每个actor发送5条消息
    for (const auto &actor : actors)
    {
        for (int i = 0; i < 5; ++i)
        {
            Message msg("test", "sender", actor->get_id());
            event_loop->deliver_message(msg);
        }
    }

    // 使用轮询调度器
    auto round_robin = std::make_shared<RoundRobinScheduler>();
    event_loop->set_scheduler(round_robin);

    // 运行事件循环一小段时间
    std::thread event_thread([&event_loop]()
                             { event_loop->run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    event_loop->stop();

    if (event_thread.joinable())
    {
        event_thread.join();
    }

    // 验证所有消息都已处理
    for (const auto &actor : actors)
    {
        assert(actor->get_message_count() == 5);
    }

    std::cout << "Scheduler test passed!" << std::endl;
}

int main()
{
    test_basic_actor();
    test_actor_communication();
    test_schedulers();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}