// Minimal, namespaced C API for eLua modules loaded by Lua 5.5.
//
// The Lua 5.5 core is private to shell_lua55.c so it can coexist with Lua
// 5.1.  Modules must use this API instead of including the Lua 5.1 headers.

#ifndef ELUA_LUA55_H
#define ELUA_LUA55_H

typedef struct lua_State elua_lua55_State;
typedef int ( *elua_lua55_CFunction )( elua_lua55_State* L );

void elua_lua55_newtable( elua_lua55_State* L );
void elua_lua55_pushcfunction( elua_lua55_State* L, elua_lua55_CFunction f );
void elua_lua55_pushstring( elua_lua55_State* L, const char* value );
void elua_lua55_pushinteger( elua_lua55_State* L, long long value );
void elua_lua55_setfield( elua_lua55_State* L, int index, const char* key );
const char* elua_lua55_checkstring( elua_lua55_State* L, int index );
int elua_lua55_error( elua_lua55_State* L, const char* message );

int elua_lua55open_pd( elua_lua55_State* L );
int elua_lua55open_elua( elua_lua55_State* L );
int elua_lua55open_cpu( elua_lua55_State* L );

#endif
