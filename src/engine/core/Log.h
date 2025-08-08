#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <iostream>

namespace nova {
class Log {
public:
    static void Init();
    static void Write(const char* level, const std::string& msg);
private:
    static std::ofstream s_file;
    static std::mutex s_mutex;
};
}
#define NOVA_LOG(lvl, msg) ::nova::Log::Write(lvl, (msg))
#define NOVA_INFO(msg) NOVA_LOG("INFO", msg)
#define NOVA_WARN(msg) NOVA_LOG("WARN", msg)
#define NOVA_ERROR(msg) NOVA_LOG("ERROR", msg)
#define NOVA_FATAL(msg) NOVA_LOG("FATAL", msg)
