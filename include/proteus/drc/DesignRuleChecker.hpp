#pragma once

#include "proteus/core/Circuit.hpp"

#include <string>
#include <vector>

namespace proteus {

enum class DrcSeverity {
    Info,
    Warning,
    Error
};

struct DrcIssue {
    DrcSeverity severity{DrcSeverity::Info};
    std::string code;
    std::string message;
};

class DesignRuleChecker {
public:
    static std::vector<DrcIssue> inspect(Circuit& circuit);
};

}
