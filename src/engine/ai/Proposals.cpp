#include "Proposals.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;
namespace nova::ai {
static std::string NewId(){
    using namespace std::chrono;
    auto t = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    return "P" + std::to_string(t);
}
Proposal Proposals::CreateFromRequest(const std::string& request){
    Proposal p{};
    p.id = NewId();
    p.dir = (fs::path(".ai/proposals") / p.id).string();
    fs::create_directories(p.dir);
    p.summaryPath = (fs::path(p.dir) / "summary.md").string();
    std::ofstream s(p.summaryPath);
    s << "# Change Proposal " << p.id << "\n\n"
      << "**Rationale:** Map NL request into deterministic engine/editor actions.\n\n"
      << "**Request:**\n\n```\n" << request << "\n```\n\n"
      << "**Plan (Phase 1 mock):** Generate patch touching sample scene or scripts.\n\n"
      << "**Apply:** Wait for `APPROVE: " << p.id << "`.\n";
    p.patchPath = (fs::path(p.dir) / "patch.diff").string();
    std::ofstream d(p.patchPath);
    d <<
"diff --git a/Scripts/rotate_cube.lua b/Scripts/rotate_cube.lua\n"
"index 1111111..2222222 100644\n"
"--- a/Scripts/rotate_cube.lua\n"
"+++ b/Scripts/rotate_cube.lua\n"
"@@\n-ROTATE_DEG_PER_SEC = 90\n+ROTATE_DEG_PER_SEC = 120\n";
    return p;
}
}
