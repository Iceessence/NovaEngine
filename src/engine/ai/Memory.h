#pragma once
#include <string>
namespace nova::ai {
struct Memory {
    static void Append(const std::string& role, const std::string& content);
    static void SummarizeIfNeeded();
};
}
