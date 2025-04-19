#include "event_loop.h"
#include "actor.h"
#include "message.h"
#include "scheduler.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

EventLoop::EventLoop()
    : running_(false), scheduler_(std::make_shared<RoundRobinScheduler>())
{
}

void EventLoop::run()
{
    running_ = true;
    std::cout << "Event loop started" << std::endl;

    // 初始化所有已注册的Actor
    for (auto &[id, actor] : actors_)
    {
        if (actor->get_state() == Actor::State::CREATED)
        {
            actor->initialize();
            actor->start();
        }
        else if (actor->get_state() == Actor::State::INITIALIZED)
        {
            actor->start();
        }
    }

    while (running_ && has_work())
    {
        process_one_cycle();

        // 避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 停止所有Actor
    for (auto &[id, actor] : actors_)
    {
        if (actor->is_running())
        {
            actor->stop();
        }
    }

    std::cout << "Event loop stopped" << std::endl;
    running_ = false;
}

void EventLoop::stop()
{
    running_ = false;
}

void EventLoop::register_actor(std::shared_ptr<Actor> actor)
{
    actors_[actor->get_id()] = actor;
    std::cout << "Registered actor: " << actor->get_name()
              << " (ID: " << actor->get_id() << ")" << std::endl;

    // 如果事件循环已经在运行，则初始化并启动Actor
    if (running_)
    {
        if (actor->get_state() == Actor::State::CREATED)
        {
            actor->initialize();
            actor->start();
        }
        else if (actor->get_state() == Actor::State::INITIALIZED)
        {
            actor->start();
        }
    }
}

void EventLoop::remove_actor(const std::string &actor_id)
{
    auto it = actors_.find(actor_id);
    if (it != actors_.end())
    {
        auto actor = it->second;

        // 如果Actor正在运行，先停止它
        if (actor->is_running())
        {
            actor->stop_immediately();
        }

        actors_.erase(it);
        std::cout << "Removed actor: " << actor->get_name()
                  << " (ID: " << actor->get_id() << ")" << std::endl;
    }
}

std::shared_ptr<Actor> EventLoop::find_actor(const std::string &actor_id)
{
    auto it = actors_.find(actor_id);
    if (it != actors_.end())
    {
        return it->second;
    }
    return nullptr;
}

void EventLoop::deliver_message(const Message &message)
{
    auto target_actor = find_actor(message.get_target_id());
    if (target_actor)
    {
        if (target_actor->is_running())
        {
            target_actor->receive(message);
        }
        else
        {
            std::cerr << "Failed to deliver message: target actor not running, ID: "
                      << message.get_target_id() << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to deliver message: target actor not found, ID: "
                  << message.get_target_id() << std::endl;
    }
}

void EventLoop::set_scheduler(std::shared_ptr<Scheduler> scheduler)
{
    scheduler_ = scheduler;
}

bool EventLoop::has_work() const
{
    // 检查是否有运行中的Actor还有消息要处理
    for (const auto &[_, actor] : actors_)
    {
        if (actor->is_running() && actor->has_messages())
        {
            return true;
        }
    }

    // 检查是否有处于STOPPING状态的Actor还有消息要处理
    for (const auto &[_, actor] : actors_)
    {
        if (actor->get_state() == Actor::State::STOPPING && actor->has_messages())
        {
            return true;
        }
    }

    return false;
}

void EventLoop::process_one_cycle()
{
    // 收集所有有消息且在运行状态的actor
    std::vector<std::shared_ptr<Actor>> active_actors;
    for (const auto &[_, actor] : actors_)
    {
        if ((actor->is_running() || actor->get_state() == Actor::State::STOPPING) && actor->has_messages())
        {
            active_actors.push_back(actor);
        }
    }

    if (active_actors.empty())
    {
        return; // 没有工作要做
    }

    // 使用调度器选择下一个要处理的actor
    auto next = scheduler_->next_actor(active_actors);
    if (next)
    {
        // 处理该actor的下一条消息
        next->process_next_message();
    }
}