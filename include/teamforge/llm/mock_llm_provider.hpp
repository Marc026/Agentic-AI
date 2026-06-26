#pragma once

#include "teamforge/llm/llm_provider.hpp"

#include <string>
#include <unordered_map>

namespace teamforge::llm {

class HttpLLMProvider : public LLMProvider {
public:
    HttpLLMProvider(std::string base_url, std::string api_key, std::string model);

    Result<CompletionResponse> complete(const CompletionRequest& request) override;
    std::string name() const override { return "http"; }

private:
    std::string base_url_;
    std::string api_key_;
    std::string default_model_;
};

class MockLLMProvider : public LLMProvider {
public:
    Result<CompletionResponse> complete(const CompletionRequest& request) override;
    std::string name() const override { return "mock"; }

    void set_response_for_role(const std::string& role_hint, nlohmann::json response);

private:
    std::unordered_map<std::string, nlohmann::json> canned_responses_;
};

}  // namespace teamforge::llm
