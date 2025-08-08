#pragma once
extern "C" { struct lua_State; }
#include <string>
namespace nova {
class LuaVM {
public:
    bool Init();
    void Shutdown();
    bool RunFile(const std::string& path);
    float GetNumber(const char* global, float fallback=0.f);
private:
    lua_State* L = nullptr;
};
}
