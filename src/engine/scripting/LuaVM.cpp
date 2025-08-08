extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "LuaVM.h"
#include "engine/core/Log.h"

namespace nova {
bool LuaVM::Init(){ L = luaL_newstate(); luaL_openlibs(L); return L!=nullptr; }
void LuaVM::Shutdown(){ if(L){ lua_close(L); L=nullptr; } }
bool LuaVM::RunFile(const std::string& p){
    if (luaL_dofile(L, p.c_str()) != LUA_OK){
        NOVA_ERROR(std::string("Lua error: ") + lua_tostring(L,-1));
        lua_pop(L,1); return false;
    }
    return true;
}
float LuaVM::GetNumber(const char* name, float fb){
    lua_getglobal(L, name);
    float v = fb;
    if (lua_isnumber(L,-1)) v = (float)lua_tonumber(L,-1);
    lua_pop(L,1);
    return v;
}
}
