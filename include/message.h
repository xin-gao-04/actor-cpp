#pragma once

#include <string>
#include <memory>
#include <variant>
#include <map>
#include <any>  // C++17标准库，需要确保编译器支持
#include <chrono>

/**
 * @brief Message类 - Actor之间通信的基本单元
 * 
 * 消息包含：
 * 1. 消息类型
 * 2. 发送者ID
 * 3. 接收者ID
 * 4. 消息负载（可以是任意类型的数据）
 * 5. 时间戳
 * 6. 优先级
 */
class Message {
public:
    // 消息优先级枚举
    enum class Priority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };
    
    // 构造函数
    Message(std::string type, 
            std::string sender_id, 
            std::string target_id,
            std::map<std::string, std::any> payload = {},
            Priority priority = Priority::NORMAL);
    
    // 获取消息类型
    const std::string& get_type() const { return type_; }
    
    // 获取发送者ID
    const std::string& get_sender_id() const { return sender_id_; }
    
    // 获取接收者ID
    const std::string& get_target_id() const { return target_id_; }
    
    // 获取整个消息负载
    const std::map<std::string, std::any>& get_payload() const { return payload_; }
    
    // 获取特定键的负载数据
    template<typename T>
    T get_payload_value(const std::string& key) const {
        auto it = payload_.find(key);
        if (it != payload_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast& e) {
                throw std::runtime_error("Type mismatch for key '" + key + "': " + e.what());
            }
        }
        throw std::runtime_error("Key not found in payload: " + key);
    }
    
    // 获取特定键的负载数据（带默认值）
    template<typename T>
    T get_payload_value_or(const std::string& key, const T& default_value) const {
        auto it = payload_.find(key);
        if (it != payload_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return default_value;
            }
        }
        return default_value;
    }
    
    // 检查负载中是否包含特定键
    bool has_payload_key(const std::string& key) const {
        return payload_.find(key) != payload_.end();
    }
    
    // 获取消息创建时间戳
    std::chrono::system_clock::time_point get_created_at() const { return created_at_; }
    
    // 获取消息优先级
    Priority get_priority() const { return priority_; }
    
    // 设置消息优先级
    void set_priority(Priority priority) { priority_ = priority; }
    
    // 比较两个消息的优先级
    static bool compare_priority(const Message& a, const Message& b) {
        return static_cast<int>(a.priority_) < static_cast<int>(b.priority_);
    }

private:
    // 消息类型
    std::string type_;
    
    // 发送者ID
    std::string sender_id_;
    
    // 接收者ID
    std::string target_id_;
    
    // 消息负载
    std::map<std::string, std::any> payload_;
    
    // 消息创建时间戳
    std::chrono::system_clock::time_point created_at_;
    
    // 消息优先级
    Priority priority_;
}; 