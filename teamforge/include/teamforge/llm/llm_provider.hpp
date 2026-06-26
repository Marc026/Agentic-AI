#pragma once

#include "teamforge/core/result.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace teamforge::llm {

struct ChatMessage {
    std::string role;
    std::string content;
};

struct CompletionRequest {
    std::string model;
    std::vector<ChatMessage> messages;
    int max_tokens{4096};
    double temperature{0.2};
    nlohmann::json response_schema;
};

struct CompletionResponse {
    std::string content;
    nlohmann::json raw;
    int prompt_tokens{0};
    int completion_tokens{0};
};

class LLMProvider {
public:
    virtual ~LLMProvider() = default;
    virtual Result<CompletionResponse> complete(const CompletionRequest& request) = 0;
    virtual std::string name() const = 0;
};

}  // namespace teamforge::llm
