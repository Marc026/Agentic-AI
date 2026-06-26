#pragma once

#include "teamforge/domain/enums.hpp"
#include "teamforge/domain/types.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace teamforge::domain {

struct Artifact {
    ArtifactId id;
    TaskId task_id;
    ArtifactKind kind{ArtifactKind::Other};
    std::string path;
    std::string checksum;
    nlohmann::json metadata;

    nlohmann::json to_json() const;
    static Artifact from_json(const nlohmann::json& j);
};

}  // namespace teamforge::domain
