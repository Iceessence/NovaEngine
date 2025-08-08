#include "Log.h"
#include <filesystem>
using namespace std::chrono;

namespace nova {
std::ofstream Log::s_file; std::mutex Log::s_mutex;

static std::string nowStr(){
    auto tp = system_clock::now();
    std::time_t t = system_clock::to_time_t(tp);
    char buf[64]; std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}
void Log::Init(){
    std::filesystem::create_directories(".logs");
    s_file.open(".logs/editor.log", std::ios::out | std::ios::trunc);
    Write("INFO","Logger ready");
}
void Log::Write(const char* level, const std::string& msg){
    std::scoped_lock lk(s_mutex);
    auto line = nowStr() + " [" + level + "] " + msg + "\n";
    std::cerr << line;
    if (s_file.is_open()) s_file << line << std::flush;
}
}
