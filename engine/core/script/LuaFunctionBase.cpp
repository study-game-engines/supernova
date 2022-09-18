//
// (c) 2022 Eduardo Doria.
//

#include "LuaFunction.h"

#include "util/StringUtils.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <stdexcept>

using namespace Supernova;

LuaFunctionBase::LuaFunctionBase(lua_State *vm, const std::string &func): m_vm(vm) {
    // get the function

    //lua_getfield(m_vm, LUA_GLOBALSINDEX, func.c_str());
    lua_getglobal(m_vm, func.c_str());

    // ensure it's a function
    if (!lua_isfunction(m_vm, -1)) {
        // throw an exception; you'd use your own exception class here
        // of course, but for sake of simplicity i use runtime_error
        lua_pop(m_vm, 1);
        throw std::runtime_error("not a valid function");
    }
    // store it in registry for later use
    m_func = luaL_ref(m_vm, LUA_REGISTRYINDEX);
}

LuaFunctionBase::LuaFunctionBase(lua_State *vm): m_vm(vm) {
    // ensure it's a function
    if (!lua_isfunction(m_vm, -1)) {
        // throw an exception; you'd use your own exception class here
        // of course, but for sake of simplicity i use runtime_error
        lua_pop(m_vm, 1);
        throw std::runtime_error("not a valid function");
    }
    // store it in registry for later use
    m_func = luaL_ref(m_vm, LUA_REGISTRYINDEX);
}

LuaFunctionBase::LuaFunctionBase(const LuaFunctionBase &other): m_vm(other.m_vm) {
    // copy the registry reference
    lua_rawgeti(m_vm, LUA_REGISTRYINDEX, other.m_func);
    m_func = luaL_ref(m_vm, LUA_REGISTRYINDEX);
}

LuaFunctionBase::~LuaFunctionBase(){
    // delete the reference from registry
    luaL_unref(m_vm, LUA_REGISTRYINDEX, m_func);
}

LuaFunctionBase& LuaFunctionBase::operator=(const LuaFunctionBase &other){
    if (this != &other) {
        m_vm = other.m_vm;
        lua_rawgeti(m_vm, LUA_REGISTRYINDEX, other.m_func);
        m_func = luaL_ref(m_vm, LUA_REGISTRYINDEX);
    }
    return *this;
}

void LuaFunctionBase::push_function(lua_State *vm, int func){
    lua_rawgeti(vm, LUA_REGISTRYINDEX, func);
}

void LuaFunctionBase::call(int args, int results) {
    // call it with no return values
    int status = lua_pcall(m_vm, args, results, 0);
    if (status) {
        // call failed; throw an exception
        std::string error = lua_tostring(m_vm, -1);
        lua_pop(m_vm, 1);
        // in reality you'd want to use your own exception class here
        throw std::runtime_error(error.c_str());
    }
}

void LuaFunctionBase::push_value(lua_State *vm, int n){ 
    lua_pushinteger(vm, n); 
}

void LuaFunctionBase::push_value(lua_State *vm, float n){
    lua_pushnumber(vm, n); 
}

void LuaFunctionBase::push_value(lua_State *vm, long n){
    lua_pushnumber(vm, n); 
}

void LuaFunctionBase::push_value(lua_State *vm, bool b){
    lua_pushboolean(vm, b);
}

void LuaFunctionBase::push_value(lua_State *vm, const std::string &s){
    lua_pushstring(vm, s.c_str());
}

void LuaFunctionBase::push_value(lua_State *vm, wchar_t s){
    lua_pushstring(vm, StringUtils::toUTF8(s).c_str());
}

template <>
struct LuaFunctionBase::value_extractor<lua_Integer>{
    static lua_Integer get(lua_State *vm){
        lua_Integer val = lua_tointeger(vm, -1);
        lua_pop(vm, 1);
        return val;
    }
};

template <>
struct LuaFunctionBase::value_extractor<lua_Number>{
    static lua_Number get(lua_State *vm){
        lua_Number val = lua_tonumber(vm, -1);
        lua_pop(vm, 1);
        return val;
    }
};

#if _WIN64 || __x86_64__
template <>
struct LuaFunctionBase::value_extractor<int>{
    static int get(lua_State *vm){
        int val = lua_tointeger(vm, -1);
        lua_pop(vm, 1);
        return val;
    }
};
#endif // _64

template <>
struct LuaFunctionBase::value_extractor<bool>{
    static bool get(lua_State *vm){
        bool val = lua_toboolean(vm, -1);
        lua_pop(vm, 1);
        return val;
    }
};

template <>
struct LuaFunctionBase::value_extractor<std::string>{
    static std::string get(lua_State *vm){
        std::string val = lua_tostring(vm, -1);
        lua_pop(vm, 1);
        return val;
    }
};