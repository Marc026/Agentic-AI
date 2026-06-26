#include "teamforge/llm/mock_llm_provider.hpp"
#include "teamforge/llm/llm_provider.hpp"

#include <httplib.h>

namespace teamforge::llm {

HttpLLMProvider::HttpLLMProvider(std::string base_url, std::string api_key, std::string model)
    : base_url_(std::move(base_url)), api_key_(std::move(api_key)), default_model_(std::move(model)) {}

Result<CompletionResponse> HttpLLMProvider::complete(const CompletionRequest& request) {
    httplib::Client client(base_url_.c_str());
    client.set_connection_timeout(30, 0);
    client.set_read_timeout(120, 0);

    nlohmann::json messages = nlohmann::json::array();
    for (const auto& m : request.messages) {
        messages.push_back({{"role", m.role}, {"content", m.content}});
    }

    nlohmann::json body = {{"model", request.model.empty() ? default_model_ : request.model},
                           {"messages", messages},
                           {"max_tokens", request.max_tokens},
                           {"temperature", request.temperature}};

    httplib::Headers headers = {{"Content-Type", "application/json"}};
    if (!api_key_.empty()) headers.emplace("Authorization", "Bearer " + api_key_);

    const auto res = client.Post("/v1/chat/completions", headers, body.dump(), "application/json");
    if (!res) return Result<CompletionResponse>::err("HTTP request failed");
    if (res->status >= 400) {
        return Result<CompletionResponse>::err("LLM API error: " + std::to_string(res->status) +
                                               " " + res->body);
    }

    const auto json = nlohmann::json::parse(res->body);
    CompletionResponse response;
    response.raw = json;
    response.content = json["choices"][0]["message"]["content"].get<std::string>();
    response.prompt_tokens = json.value("usage", nlohmann::json::object()).value("prompt_tokens", 0);
    response.completion_tokens =
        json.value("usage", nlohmann::json::object()).value("completion_tokens", 0);
    return Result<CompletionResponse>::ok(std::move(response));
}

void MockLLMProvider::set_response_for_role(const std::string& role_hint, nlohmann::json response) {
    canned_responses_[role_hint] = std::move(response);
}

Result<CompletionResponse> MockLLMProvider::complete(const CompletionRequest& request) {
    std::string hint;
    for (const auto& m : request.messages) {
        if (m.role == "system") hint = m.content;
    }

    nlohmann::json payload;
    if (hint.find("Orchestrator") != std::string::npos ||
        hint.find("Project Manager") != std::string::npos) {
        payload = canned_responses_.count("Orchestrator")
                      ? canned_responses_.at("Orchestrator")
                      : nlohmann::json::parse(R"({
            "subtasks": [
              {"id":"st-arch","assigned_role":"Architect","objective":"Design URL shortener API","depends_on":[],"gate":"None"},
              {"id":"st-rev-design","assigned_role":"CodeReviewer","objective":"Review design document","depends_on":["st-arch"],"gate":"Review"},
              {"id":"st-backend","assigned_role":"Backend","objective":"Implement URL shortener service","depends_on":["st-rev-design"],"gate":"None"},
              {"id":"st-test","assigned_role":"TestEngineer","objective":"Generate and run tests","depends_on":["st-backend"],"gate":"Test"},
              {"id":"st-rev-code","assigned_role":"CodeReviewer","objective":"Review implementation","depends_on":["st-backend"],"gate":"Review"}
            ]
          })");
    } else if (hint.find("Architect") != std::string::npos) {
        payload = canned_responses_.count("Architect")
                      ? canned_responses_.at("Architect")
                      : nlohmann::json::parse(R"({
            "title":"URL Shortener API",
            "modules":["api","storage","encoding"],
            "endpoints":[
              {"method":"POST","path":"/api/shorten","description":"Create short URL"},
              {"method":"GET","path":"/{code}","description":"Redirect to original URL"}
            ],
            "data_models":[
              {"name":"ShortLink","fields":["code","original_url","created_at"]}
            ],
            "storage":"in-memory map with optional SQLite persistence",
            "adrs":[{"decision":"Use base62 codes","rationale":"URL-friendly short codes"}],
            "non_goals":["User authentication","Analytics dashboard"]
          })");
    } else if (hint.find("Backend") != std::string::npos) {
        payload = canned_responses_.count("Backend")
                      ? canned_responses_.at("Backend")
                      : nlohmann::json::parse(R"({
            "files":[
              {"path":"src/url_shortener.hpp","content":"#pragma once\n#include <string>\n#include <unordered_map>\nclass UrlShortener {\npublic:\n  std::string shorten(const std::string& url);\n  std::string resolve(const std::string& code) const;\nprivate:\n  std::unordered_map<std::string,std::string> links_;\n};\n"},
              {"path":"src/url_shortener.cpp","content":"#include \"url_shortener.hpp\"\n#include <random>\nstatic std::string gen_code() { return \"abc123\"; }\nstd::string UrlShortener::shorten(const std::string& url) {\n  const auto code = gen_code();\n  links_[code] = url;\n  return code;\n}\nstd::string UrlShortener::resolve(const std::string& code) const {\n  auto it = links_.find(code);\n  return it == links_.end() ? \"\" : it->second;\n}\n"}
            ],
            "build_instructions":"Add sources to CMakeLists and build"
          })");
    } else if (hint.find("Test") != std::string::npos) {
        payload = canned_responses_.count("TestEngineer")
                      ? canned_responses_.at("TestEngineer")
                      : nlohmann::json::parse(R"({
            "test_files":[
              {"path":"tests/test_url_shortener.cpp","content":"#include \"url_shortener.hpp\"\n#include <cassert>\nint main(){ UrlShortener s; auto c=s.shorten(\"http://example.com\"); assert(!c.empty()); assert(s.resolve(c)==\"http://example.com\"); return 0;}\n"}
            ],
            "test_command":"echo tests-passed",
            "passed":true,
            "coverage_estimate":0.85
          })");
    } else if (hint.find("Reviewer") != std::string::npos || hint.find("Code Reviewer") != std::string::npos) {
        payload = canned_responses_.count("CodeReviewer")
                      ? canned_responses_.at("CodeReviewer")
                      : nlohmann::json::parse(R"({
            "verdict":"Approve",
            "score":0.9,
            "findings":[
              {"severity":"Info","message":"Consider collision handling for code generation","location":"url_shortener.cpp"}
            ]
          })");
    } else {
        payload = {{"status", "ok"}};
    }

    CompletionResponse response;
    response.content = payload.dump();
    response.raw = payload;
    return Result<CompletionResponse>::ok(std::move(response));
}

}  // namespace teamforge::llm
