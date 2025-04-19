#include "message.h"

Message::Message(std::string type, 
                 std::string sender_id, 
                 std::string target_id,
                 std::map<std::string, std::any> payload,
                 Priority priority)
    : type_(std::move(type))
    , sender_id_(std::move(sender_id))
    , target_id_(std::move(target_id))
    , payload_(std::move(payload))
    , created_at_(std::chrono::system_clock::now())
    , priority_(priority) {
} 