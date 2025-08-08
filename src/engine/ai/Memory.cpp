#include "Memory.h"
#include "engine/core/Log.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;
namespace nova::ai {
static fs::path MemRoot(){ return fs::path(".ai/memory"); }

static std::string isoTime(){
    using namespace std::chrono;
    auto t = system_clock::to_time_t(system_clock::now());
    std::tm tm; localtime_s(&tm, &t);
    char buf[32]; std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

void Memory::Append(const std::string& role, const std::string& content){
    fs::create_directories(MemRoot());
    std::ofstream f(MemRoot()/"conversation.jsonl", std::ios::app);
    std::string esc = content; // naive escape for phase 1
    for (char& c : esc) if (c=='\"') c = '\'';
    f << "{\"ts\":\"" << isoTime() << "\",\"role\":\"" << role << "\",\"content\":\"" << esc << "\"}\n";
    NOVA_INFO("AI memory appended (" + role + ")");
}
void Memory::SummarizeIfNeeded(){
    fs::create_directories(MemRoot()/ "summaries");
}
}
