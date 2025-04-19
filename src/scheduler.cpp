#include "scheduler.h"
#include "actor.h"
#include "message.h"
#include <algorithm>
#include <limits>
#include <iostream>

// RoundRobinScheduler Implementation
RoundRobinScheduler::RoundRobinScheduler() : current_index_(0) {}

std::shared_ptr<Actor> RoundRobinScheduler::next_actor(const std::vector<std::shared_ptr<Actor>>& actors) {
    if (actors.empty()) {
        return nullptr;
    }
    
    // 确保索引在有效范围内
    if (current_index_ >= actors.size()) {
        current_index_ = 0;
    }
    
    // 获取当前索引的actor
    auto actor = actors[current_index_];
    
    // 更新索引，准备下一次调度
    current_index_ = (current_index_ + 1) % actors.size();
    
    return actor;
}

// PriorityScheduler Implementation
PriorityScheduler::PriorityScheduler(PriorityFunction priority_func)
    : priority_func_(priority_func ? std::move(priority_func) : 
                    [this](const std::shared_ptr<Actor>& actor) { 
                        return this->default_priority(actor); 
                    }) {}

std::shared_ptr<Actor> PriorityScheduler::next_actor(const std::vector<std::shared_ptr<Actor>>& actors) {
    if (actors.empty()) {
        return nullptr;
    }
    
    // 找到具有最高优先级的actor
    auto highest_priority_actor = *std::max_element(
        actors.begin(), actors.end(),
        [this](const std::shared_ptr<Actor>& a, const std::shared_ptr<Actor>& b) {
            return priority_func_(a) < priority_func_(b);
        }
    );
    
    return highest_priority_actor;
}

int PriorityScheduler::default_priority(const std::shared_ptr<Actor>& actor) {
    // 默认优先级实现：消息队列中的消息数量越多，优先级越高
    // 可以扩展为考虑其他因素，如消息的年龄、类型等
    return actor->has_messages() ? 1 : 0;
}

// MessagePriorityScheduler Implementation
MessagePriorityScheduler::MessagePriorityScheduler() {}

std::shared_ptr<Actor> MessagePriorityScheduler::next_actor(const std::vector<std::shared_ptr<Actor>>& actors) {
    if (actors.empty()) {
        return nullptr;
    }
    
    // 找到具有最高优先级消息的actor
    auto highest_priority_actor = *std::max_element(
        actors.begin(), actors.end(),
        [this](const std::shared_ptr<Actor>& a, const std::shared_ptr<Actor>& b) {
            Message msg_a = peek_highest_priority_message(a);
            Message msg_b = peek_highest_priority_message(b);
            return static_cast<int>(msg_a.get_priority()) < static_cast<int>(msg_b.get_priority());
        }
    );
    
    return highest_priority_actor;
}

Message MessagePriorityScheduler::peek_highest_priority_message(const std::shared_ptr<Actor>& actor) {
    // 使用Actor的peek_highest_priority_message方法
    return actor->peek_highest_priority_message();
}

// FairScheduler Implementation
FairScheduler::FairScheduler(std::chrono::milliseconds max_starvation_time)
    : max_starvation_time_(max_starvation_time) {}

std::shared_ptr<Actor> FairScheduler::next_actor(const std::vector<std::shared_ptr<Actor>>& actors) {
    if (actors.empty()) {
        return nullptr;
    }
    
    auto now = std::chrono::system_clock::now();
    
    // 首先检查是否有长时间未被调度的Actor
    for (const auto& actor : actors) {
        auto it = last_scheduled_.find(actor->get_id());
        
        // 如果这个Actor之前没有被调度过，或者已经超过了最大饥饿时间
        if (it == last_scheduled_.end() || 
            now - it->second > max_starvation_time_) {
            // 更新最后调度时间
            last_scheduled_[actor->get_id()] = now;
            return actor;
        }
    }
    
    // 如果没有饥饿的Actor，则选择等待时间最长的一个
    auto oldest_scheduled = *std::min_element(
        actors.begin(), actors.end(),
        [this](const std::shared_ptr<Actor>& a, const std::shared_ptr<Actor>& b) {
            auto it_a = last_scheduled_.find(a->get_id());
            auto it_b = last_scheduled_.find(b->get_id());
            
            // 如果某个Actor从未被调度过，给它最高优先级
            if (it_a == last_scheduled_.end()) return true;
            if (it_b == last_scheduled_.end()) return false;
            
            return it_a->second < it_b->second;
        }
    );
    
    // 更新最后调度时间
    last_scheduled_[oldest_scheduled->get_id()] = now;
    
    return oldest_scheduled;
} 