#pragma once
#include <string>
namespace nova::ai {
struct Proposal {
    std::string id;
    std::string dir;
    std::string summaryPath;
    std::string patchPath;
};
struct Proposals {
    static Proposal CreateFromRequest(const std::string& request);
};
}
