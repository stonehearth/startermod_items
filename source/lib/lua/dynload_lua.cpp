#include "pch.h"
#include "core/config.h"
#include "core/system.h"
#include "caching_allocator.h"

extern "C" {
#  include "lib/lua/lua.h"
}

using namespace radiant;
using namespace radiant::lua;

static bool __jitEnabled;

HMODULE luadll;
static void *LoadSymbol(const char* name)
{
   return GetProcAddress(luadll, name);
}

bool lua::JitIsEnabled()
{
   return __jitEnabled;
}

static lua_State * (__cdecl *lua_newstate_fn)(lua_Alloc f, void *ud);
static void (__cdecl *lua_close_fn)(lua_State *L);
static lua_State * (__cdecl *lua_newthread_fn)(lua_State *L);
static lua_CFunction (__cdecl *lua_atpanic_fn)(lua_State *L, lua_CFunction panicf);
static int (__cdecl *lua_gettop_fn)(lua_State *L);
static void (__cdecl *lua_settop_fn)(lua_State *L, int idx);
static void (__cdecl *lua_pushvalue_fn)(lua_State *L, int idx);
static void (__cdecl *lua_remove_fn)(lua_State *L, int idx);
static void (__cdecl *lua_insert_fn)(lua_State *L, int idx);
static void (__cdecl *lua_replace_fn)(lua_State *L, int idx);
static int (__cdecl *lua_checkstack_fn)(lua_State *L, int sz);
static void (__cdecl *lua_xmove_fn)(lua_State *from, lua_State *to, int n);
static int (__cdecl *lua_isnumber_fn)(lua_State *L, int idx);
static int (__cdecl *lua_isstring_fn)(lua_State *L, int idx);
static int (__cdecl *lua_iscfunction_fn)(lua_State *L, int idx);
static int (__cdecl *lua_isuserdata_fn)(lua_State *L, int idx);
static int (__cdecl *lua_type_fn)(lua_State *L, int idx);
static const char     * (__cdecl *lua_typename_fn)(lua_State *L, int tp);
static int (__cdecl *lua_equal_fn)(lua_State *L, int idx1, int idx2);
static int (__cdecl *lua_rawequal_fn)(lua_State *L, int idx1, int idx2);
static int (__cdecl *lua_lessthan_fn)(lua_State *L, int idx1, int idx2);
static lua_Number (__cdecl *lua_tonumber_fn)(lua_State *L, int idx);
static lua_Integer (__cdecl *lua_tointeger_fn)(lua_State *L, int idx);
static int (__cdecl *lua_toboolean_fn)(lua_State *L, int idx);
static const char     * (__cdecl *lua_tolstring_fn)(lua_State *L, int idx, size_t *len);
static size_t (__cdecl *lua_objlen_fn)(lua_State *L, int idx);
static lua_CFunction (__cdecl *lua_tocfunction_fn)(lua_State *L, int idx);
static void          * (__cdecl *lua_touserdata_fn)(lua_State *L, int idx);
static lua_State      * (__cdecl *lua_tothread_fn)(lua_State *L, int idx);
static const void     * (__cdecl *lua_topointer_fn)(lua_State *L, int idx);
static void (__cdecl *lua_pushnil_fn)(lua_State *L);
static void (__cdecl *lua_pushnumber_fn)(lua_State *L, lua_Number n);
static void (__cdecl *lua_pushinteger_fn)(lua_State *L, lua_Integer n);
static void (__cdecl *lua_pushlstring_fn)(lua_State *L, const char *s, size_t l);
static void (__cdecl *lua_pushstring_fn)(lua_State *L, const char *s);
static const char * (__cdecl *lua_pushvfstring_fn)(lua_State *L, const char *fmt, va_list argp);
static void (__cdecl *lua_pushcclosure_fn)(lua_State *L, lua_CFunction fn, int n);
static void (__cdecl *lua_pushboolean_fn)(lua_State *L, int b);
static void (__cdecl *lua_pushlightuserdata_fn)(lua_State *L, void *p);
static int (__cdecl *lua_pushthread_fn)(lua_State *L);
static void (__cdecl *lua_gettable_fn)(lua_State *L, int idx);
static void (__cdecl *lua_getfield_fn)(lua_State *L, int idx, const char *k);
static void (__cdecl *lua_rawget_fn)(lua_State *L, int idx);
static void (__cdecl *lua_rawgeti_fn)(lua_State *L, int idx, int n);
static void (__cdecl *lua_createtable_fn)(lua_State *L, int narr, int nrec);
static void * (__cdecl *lua_newuserdata_fn)(lua_State *L, size_t sz);
static int (__cdecl *lua_getmetatable_fn)(lua_State *L, int objindex);
static void (__cdecl *lua_getfenv_fn)(lua_State *L, int idx);
static void (__cdecl *lua_settable_fn)(lua_State *L, int idx);
static void (__cdecl *lua_setfield_fn)(lua_State *L, int idx, const char *k);
static void (__cdecl *lua_rawset_fn)(lua_State *L, int idx);
static void (__cdecl *lua_rawseti_fn)(lua_State *L, int idx, int n);
static int (__cdecl *lua_setmetatable_fn)(lua_State *L, int objindex);
static int (__cdecl *lua_setfenv_fn)(lua_State *L, int idx);
static void (__cdecl *lua_call_fn)(lua_State *L, int nargs, int nresults);
static int (__cdecl *lua_pcall_fn)(lua_State *L, int nargs, int nresults, int errfunc);
static int (__cdecl *lua_cpcall_fn)(lua_State *L, lua_CFunction func, void *ud);
static int (__cdecl *lua_load_fn)(lua_State *L, lua_Reader reader, void *dt, const char *chunkname);
static int (__cdecl *lua_dump_fn)(lua_State *L, lua_Writer writer, void *data);
static int (__cdecl *lua_yield_fn)(lua_State *L, int nresults);
static int (__cdecl *lua_resume_fn)(lua_State *L, int narg);
static int (__cdecl *lua_status_fn)(lua_State *L);
static int (__cdecl *lua_error_fn)(lua_State *L);
static int (__cdecl *lua_gc_fn)(lua_State *L, int what, int data);
static int (__cdecl *lua_next_fn)(lua_State *L, int idx);
static void (__cdecl *lua_concat_fn)(lua_State *L, int n);
static lua_Alloc (__cdecl *lua_getallocf_fn)(lua_State *L, void **ud);
static void (__cdecl *lua_setallocf_fn)(lua_State *L, lua_Alloc f, void *ud);
static lua_Alloc2 (__cdecl *lua_getalloc2f_fn)(lua_State *L, void **ud);
static void (__cdecl *lua_setalloc2f_fn)(lua_State *L, lua_Alloc2 f, void *ud);
static void (__cdecl *lua_setlevel_fn)(lua_State *from, lua_State *to);
static int (__cdecl *lua_getstack_fn)(lua_State *L, int level, lua_Debug *ar);
static int (__cdecl *lua_getinfo_fn)(lua_State *L, const char *what, lua_Debug *ar);
static const char * (__cdecl *lua_getlocal_fn)(lua_State *L, const lua_Debug *ar, int n);
static const char * (__cdecl *lua_setlocal_fn)(lua_State *L, const lua_Debug *ar, int n);
static const char * (__cdecl *lua_getupvalue_fn)(lua_State *L, int funcindex, int n);
static const char * (__cdecl *lua_setupvalue_fn)(lua_State *L, int funcindex, int n);
static int (__cdecl *lua_sethook_fn)(lua_State *L, lua_Hook func, int mask, int count);
static lua_Hook (__cdecl *lua_gethook_fn)(lua_State *L);
static int (__cdecl *lua_gethookmask_fn)(lua_State *L);
static int (__cdecl *lua_gethookcount_fn)(lua_State *L);
static void (__cdecl *luaI_openlib_fn)(lua_State *L, const char *libname, const luaL_Reg *l, int nup);
static void (__cdecl *luaL_register_fn)(lua_State *L, const char *libname, const luaL_Reg *l);
static int (__cdecl *luaL_getmetafield_fn)(lua_State *L, int obj, const char *e);
static int (__cdecl *luaL_callmeta_fn)(lua_State *L, int obj, const char *e);
static int (__cdecl *luaL_typerror_fn)(lua_State *L, int narg, const char *tname);
static int (__cdecl *luaL_argerror_fn)(lua_State *L, int numarg, const char *extramsg);
static const char * (__cdecl *luaL_checklstring_fn)(lua_State *L, int numArg, size_t *l);
static const char * (__cdecl *luaL_optlstring_fn)(lua_State *L, int numArg, const char *def, size_t *l);
static lua_Number (__cdecl *luaL_checknumber_fn)(lua_State *L, int numArg);
static lua_Number (__cdecl *luaL_optnumber_fn)(lua_State *L, int nArg, lua_Number def);
static lua_Integer (__cdecl *luaL_checkinteger_fn)(lua_State *L, int numArg);
static lua_Integer (__cdecl *luaL_optinteger_fn)(lua_State *L, int nArg, lua_Integer def);
static void (__cdecl *luaL_checkstack_fn)(lua_State *L, int sz, const char *msg);
static void (__cdecl *luaL_checktype_fn)(lua_State *L, int narg, int t);
static void (__cdecl *luaL_checkany_fn)(lua_State *L, int narg);
static int (__cdecl *luaL_newmetatable_fn)(lua_State *L, const char *tname);
static void * (__cdecl *luaL_checkudata_fn)(lua_State *L, int ud, const char *tname);
static void (__cdecl *luaL_where_fn)(lua_State *L, int lvl);
static int (__cdecl *luaL_checkoption_fn)(lua_State *L, int narg, const char *def, const char *const lst[]);
static int (__cdecl *luaL_ref_fn)(lua_State *L, int t);
static void (__cdecl *luaL_unref_fn)(lua_State *L, int t, int ref);
static int (__cdecl *luaL_loadfile_fn)(lua_State *L, const char *filename);
static int (__cdecl *luaL_loadbuffer_fn)(lua_State *L, const char *buff, size_t sz, const char *name);
static int (__cdecl *luaL_loadstring_fn)(lua_State *L, const char *s);
static lua_State * (__cdecl *luaL_newstate_fn)(void);
static const char * (__cdecl *luaL_gsub_fn)(lua_State *L, const char *s, const char *p, const char *r);
static const char * (__cdecl *luaL_findtable_fn)(lua_State *L, int idx, const char *fname, int szhint);
static void (__cdecl *luaL_openlibs_fn)(lua_State *L);
static lua_State *(__cdecl *lj_state_newstate_fn)(lua_Alloc f, void *ud);
static int (*luaL_error_fn) (lua_State *L, const char *fmt, ...);

void lua::Initialize()
{
   bool is64Bit = core::System::IsProcess64Bit();
   bool enableJit = core::Config::GetInstance().Get<bool>("enable_lua_jit", true);

   // xxx: temporary code idea: disable luajit entirely if we're in 64-bit mode.
   if (enableJit && is64Bit && !core::Config::GetInstance().Get<bool>("force_lua_jit", false)) {
      LOG(lua.code, 0) << "lua jit disabled in 64-bit builds.  set force_lua_jit to override";
      enableJit = false;
   }
   
   // So, it turns out we cannot let the GC run at certain times (e.g. when steaming), since
   // we cannot limit the side-effects of the GC.  So, until we can port the HIBERNATE flag
   // into luajit (or find some other way to say "by no means are you allowed to GC in this
   // interval), we cannot use it.  AGHAHGH! -- tony
   enableJit = false;

   CachingAllocator &la = CachingAllocator::GetInstance();

   bool useLowMemory = enableJit && is64Bit;
   la.Start(useLowMemory);
   if (useLowMemory && !la.IsUsingLowMemory()) {
      LOG(lua.memory, 0) << "failed to allocate low memory.  disabling lua jit!";
      enableJit = false;
   }

   __jitEnabled = enableJit;
   luadll = LoadLibraryExW(__jitEnabled ? L"lua-5.1.5.jit.dll" : L"lua-5.1.5.dll", NULL, 0);

   LOG(lua.data, 0) << "lua jit is " << (__jitEnabled ? "enabled" : "disabled");

   lua_newstate_fn = (lua_State *(*)(lua_Alloc f, void *ud))LoadSymbol("lua_newstate");
   lua_close_fn = (void(*)(lua_State *L))LoadSymbol("lua_close");
   lua_newthread_fn = (lua_State *(*)(lua_State *L))LoadSymbol("lua_newthread");
   lua_atpanic_fn = (lua_CFunction(*)(lua_State *L, lua_CFunction panicf))LoadSymbol("lua_atpanic");
   lua_gettop_fn = (int(*)(lua_State *L))LoadSymbol("lua_gettop");
   lua_settop_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_settop");
   lua_pushvalue_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_pushvalue");
   lua_remove_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_remove");
   lua_insert_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_insert");
   lua_replace_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_replace");
   lua_checkstack_fn = (int(*)(lua_State *L, int sz))LoadSymbol("lua_checkstack");
   lua_xmove_fn = (void(*)(lua_State *from, lua_State *to, int n))LoadSymbol("lua_xmove");
   lua_isnumber_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_isnumber");
   lua_isstring_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_isstring");
   lua_iscfunction_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_iscfunction");
   lua_isuserdata_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_isuserdata");
   lua_type_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_type");
   lua_typename_fn = (const char     *(*)(lua_State *L, int tp))LoadSymbol("lua_typename");
   lua_equal_fn = (int(*)(lua_State *L, int idx1, int idx2))LoadSymbol("lua_equal");
   lua_rawequal_fn = (int(*)(lua_State *L, int idx1, int idx2))LoadSymbol("lua_rawequal");
   lua_lessthan_fn = (int(*)(lua_State *L, int idx1, int idx2))LoadSymbol("lua_lessthan");
   lua_tonumber_fn = (lua_Number(*)(lua_State *L, int idx))LoadSymbol("lua_tonumber");
   lua_tointeger_fn = (lua_Integer(*)(lua_State *L, int idx))LoadSymbol("lua_tointeger");
   lua_toboolean_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_toboolean");
   lua_tolstring_fn = (const char     *(*)(lua_State *L, int idx, size_t *len))LoadSymbol("lua_tolstring");
   lua_objlen_fn = (size_t(*)(lua_State *L, int idx))LoadSymbol("lua_objlen");
   lua_tocfunction_fn = (lua_CFunction(*)(lua_State *L, int idx))LoadSymbol("lua_tocfunction");
   lua_touserdata_fn = (void          *(*)(lua_State *L, int idx))LoadSymbol("lua_touserdata");
   lua_tothread_fn = (lua_State      *(*)(lua_State *L, int idx))LoadSymbol("lua_tothread");
   lua_topointer_fn = (const void     *(*)(lua_State *L, int idx))LoadSymbol("lua_topointer");
   lua_pushnil_fn = (void(*)(lua_State *L))LoadSymbol("lua_pushnil");
   lua_pushnumber_fn = (void(*)(lua_State *L, lua_Number n))LoadSymbol("lua_pushnumber");
   lua_pushinteger_fn = (void(*)(lua_State *L, lua_Integer n))LoadSymbol("lua_pushinteger");
   lua_pushlstring_fn = (void(*)(lua_State *L, const char *s, size_t l))LoadSymbol("lua_pushlstring");
   lua_pushstring_fn = (void(*)(lua_State *L, const char *s))LoadSymbol("lua_pushstring");
   lua_pushvfstring_fn = (const char *(*)(lua_State *L, const char *fmt, va_list argp))LoadSymbol("lua_pushvfstring");
   lua_pushcclosure_fn = (void(*)(lua_State *L, lua_CFunction fn, int n))LoadSymbol("lua_pushcclosure");
   lua_pushboolean_fn = (void(*)(lua_State *L, int b))LoadSymbol("lua_pushboolean");
   lua_pushlightuserdata_fn = (void(*)(lua_State *L, void *p))LoadSymbol("lua_pushlightuserdata");
   lua_pushthread_fn = (int(*)(lua_State *L))LoadSymbol("lua_pushthread");
   lua_gettable_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_gettable");
   lua_getfield_fn = (void(*)(lua_State *L, int idx, const char *k))LoadSymbol("lua_getfield");
   lua_rawget_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_rawget");
   lua_rawgeti_fn = (void(*)(lua_State *L, int idx, int n))LoadSymbol("lua_rawgeti");
   lua_createtable_fn = (void(*)(lua_State *L, int narr, int nrec))LoadSymbol("lua_createtable");
   lua_newuserdata_fn = (void *(*)(lua_State *L, size_t sz))LoadSymbol("lua_newuserdata");
   lua_getmetatable_fn = (int(*)(lua_State *L, int objindex))LoadSymbol("lua_getmetatable");
   lua_getfenv_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_getfenv");
   lua_settable_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_settable");
   lua_setfield_fn = (void(*)(lua_State *L, int idx, const char *k))LoadSymbol("lua_setfield");
   lua_rawset_fn = (void(*)(lua_State *L, int idx))LoadSymbol("lua_rawset");
   lua_rawseti_fn = (void(*)(lua_State *L, int idx, int n))LoadSymbol("lua_rawseti");
   lua_setmetatable_fn = (int(*)(lua_State *L, int objindex))LoadSymbol("lua_setmetatable");
   lua_setfenv_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_setfenv");
   lua_call_fn = (void(*)(lua_State *L, int nargs, int nresults))LoadSymbol("lua_call");
   lua_pcall_fn = (int(*)(lua_State *L, int nargs, int nresults, int errfunc))LoadSymbol("lua_pcall");
   lua_cpcall_fn = (int(*)(lua_State *L, lua_CFunction func, void *ud))LoadSymbol("lua_cpcall");
   lua_load_fn = (int(*)(lua_State *L, lua_Reader reader, void *dt, const char *chunkname))LoadSymbol("lua_load");
   lua_dump_fn = (int(*)(lua_State *L, lua_Writer writer, void *data))LoadSymbol("lua_dump");
   lua_yield_fn = (int(*)(lua_State *L, int nresults))LoadSymbol("lua_yield");
   lua_resume_fn = (int(*)(lua_State *L, int narg))LoadSymbol("lua_resume");
   lua_status_fn = (int(*)(lua_State *L))LoadSymbol("lua_status");
   lua_error_fn = (int(*)(lua_State *L))LoadSymbol("lua_error");
   lua_gc_fn = (int(*)(lua_State *L, int what, int data))LoadSymbol("lua_gc");
   lua_next_fn = (int(*)(lua_State *L, int idx))LoadSymbol("lua_next");
   lua_concat_fn = (void(*)(lua_State *L, int n))LoadSymbol("lua_concat");
   lua_getallocf_fn = (lua_Alloc(*)(lua_State *L, void **ud))LoadSymbol("lua_getallocf");
   lua_setallocf_fn = (void(*)(lua_State *L, lua_Alloc f, void *ud))LoadSymbol("lua_setallocf");
   lua_getalloc2f_fn = (lua_Alloc2(*)(lua_State *L, void **ud))LoadSymbol("lua_getalloc2f");
   lua_setalloc2f_fn = (void(*)(lua_State *L, lua_Alloc2 f, void *ud))LoadSymbol("lua_setalloc2f");
   lua_setlevel_fn = (void(*)(lua_State *from, lua_State *to))LoadSymbol("lua_setlevel");
   lua_getstack_fn = (int(*)(lua_State *L, int level, lua_Debug *ar))LoadSymbol("lua_getstack");
   lua_getinfo_fn = (int(*)(lua_State *L, const char *what, lua_Debug *ar))LoadSymbol("lua_getinfo");
   lua_getlocal_fn = (const char *(*)(lua_State *L, const lua_Debug *ar, int n))LoadSymbol("lua_getlocal");
   lua_setlocal_fn = (const char *(*)(lua_State *L, const lua_Debug *ar, int n))LoadSymbol("lua_setlocal");
   lua_getupvalue_fn = (const char *(*)(lua_State *L, int funcindex, int n))LoadSymbol("lua_getupvalue");
   lua_setupvalue_fn = (const char *(*)(lua_State *L, int funcindex, int n))LoadSymbol("lua_setupvalue");
   lua_sethook_fn = (int(*)(lua_State *L, lua_Hook func, int mask, int count))LoadSymbol("lua_sethook");
   lua_gethook_fn = (lua_Hook(*)(lua_State *L))LoadSymbol("lua_gethook");
   lua_gethookmask_fn = (int(*)(lua_State *L))LoadSymbol("lua_gethookmask");
   lua_gethookcount_fn = (int(*)(lua_State *L))LoadSymbol("lua_gethookcount");
   luaI_openlib_fn = (void(*)(lua_State *L, const char *libname, const luaL_Reg *l, int nup))LoadSymbol("luaI_openlib");
   luaL_register_fn = (void(*)(lua_State *L, const char *libname, const luaL_Reg *l))LoadSymbol("luaL_register");
   luaL_getmetafield_fn = (int(*)(lua_State *L, int obj, const char *e))LoadSymbol("luaL_getmetafield");
   luaL_callmeta_fn = (int(*)(lua_State *L, int obj, const char *e))LoadSymbol("luaL_callmeta");
   luaL_typerror_fn = (int(*)(lua_State *L, int narg, const char *tname))LoadSymbol("luaL_typerror");
   luaL_argerror_fn = (int(*)(lua_State *L, int numarg, const char *extramsg))LoadSymbol("luaL_argerror");
   luaL_checklstring_fn = (const char *(*)(lua_State *L, int numArg, size_t *l))LoadSymbol("luaL_checklstring");
   luaL_optlstring_fn = (const char *(*)(lua_State *L, int numArg, const char *def, size_t *l))LoadSymbol("luaL_optlstring");
   luaL_checknumber_fn = (lua_Number(*)(lua_State *L, int numArg))LoadSymbol("luaL_checknumber");
   luaL_optnumber_fn = (lua_Number(*)(lua_State *L, int nArg, lua_Number def))LoadSymbol("luaL_optnumber");
   luaL_checkinteger_fn = (lua_Integer(*)(lua_State *L, int numArg))LoadSymbol("luaL_checkinteger");
   luaL_optinteger_fn = (lua_Integer(*)(lua_State *L, int nArg, lua_Integer def))LoadSymbol("luaL_optinteger");
   luaL_checkstack_fn = (void(*)(lua_State *L, int sz, const char *msg))LoadSymbol("luaL_checkstack");
   luaL_checktype_fn = (void(*)(lua_State *L, int narg, int t))LoadSymbol("luaL_checktype");
   luaL_checkany_fn = (void(*)(lua_State *L, int narg))LoadSymbol("luaL_checkany");
   luaL_newmetatable_fn = (int(*)(lua_State *L, const char *tname))LoadSymbol("luaL_newmetatable");
   luaL_checkudata_fn = (void *(*)(lua_State *L, int ud, const char *tname))LoadSymbol("luaL_checkudata");
   luaL_where_fn = (void(*)(lua_State *L, int lvl))LoadSymbol("luaL_where");
   luaL_checkoption_fn = (int(*)(lua_State *L, int narg, const char *def, const char *const lst[]))LoadSymbol("luaL_checkoption");
   luaL_ref_fn = (int(*)(lua_State *L, int t))LoadSymbol("luaL_ref");
   luaL_unref_fn = (void(*)(lua_State *L, int t, int ref))LoadSymbol("luaL_unref");
   luaL_loadfile_fn = (int(*)(lua_State *L, const char *filename))LoadSymbol("luaL_loadfile");
   luaL_loadbuffer_fn = (int(*)(lua_State *L, const char *buff, size_t sz, const char *name))LoadSymbol("luaL_loadbuffer");
   luaL_loadstring_fn = (int(*)(lua_State *L, const char *s))LoadSymbol("luaL_loadstring");
   luaL_newstate_fn = (lua_State *(*)(void))LoadSymbol("luaL_newstate");
   luaL_gsub_fn = (const char *(*)(lua_State *L, const char *s, const char *p, const char *r))LoadSymbol("luaL_gsub");
   luaL_findtable_fn = (const char *(*)(lua_State *L, int idx, const char *fname, int szhint))LoadSymbol("luaL_findtable");
   luaL_openlibs_fn = (void(*)(lua_State *L))LoadSymbol("luaL_openlibs");
   lj_state_newstate_fn = (lua_State *(*)(lua_Alloc f, void *ud))LoadSymbol("lj_state_newstate");

   luaL_error_fn = (int(*)(lua_State *L, const char *fmt, ...))LoadSymbol("luaL_error_fn");
}

extern "C" lua_State * lua_newstate(lua_Alloc f, void *ud)
{
   if (lj_state_newstate_fn) {
      return lj_state_newstate_fn(f, ud);
   }

   ASSERT(lua_newstate_fn);
   if (lua_newstate_fn) {
      return (*lua_newstate_fn)(f, ud);
   }
   return NULL;
}

extern "C" lua_State * lj_state_newstate(lua_Alloc f, void *ud)
{
   ASSERT(lj_state_newstate_fn);
   if (lj_state_newstate_fn) {
      return lj_state_newstate_fn(f, ud);
   }
   return NULL;
}

extern "C" void lua_close(lua_State *L)
{
   ASSERT(lua_close_fn);
   if (lua_close_fn) {
      (*lua_close_fn)(L);
   }
}

extern "C" lua_State * lua_newthread(lua_State *L)
{
   ASSERT(lua_newthread_fn);
   if (lua_newthread_fn) {
      return (*lua_newthread_fn)(L);
   }
   return NULL;
}

extern "C" lua_CFunction lua_atpanic(lua_State *L, lua_CFunction panicf)
{
   ASSERT(lua_atpanic_fn);
   if (lua_atpanic_fn) {
      return (*lua_atpanic_fn)(L, panicf);
   }
   return NULL;
}

extern "C" int lua_gettop(lua_State *L)
{
   ASSERT(lua_gettop_fn);
   if (lua_gettop_fn) {
      return (*lua_gettop_fn)(L);
   }
   return 0;
}

extern "C" void lua_settop(lua_State *L, int idx)
{
   ASSERT(lua_settop_fn);
   if (lua_settop_fn) {
      (*lua_settop_fn)(L, idx);
   }
}

extern "C" void lua_pushvalue(lua_State *L, int idx)
{
   ASSERT(lua_pushvalue_fn);
   if (lua_pushvalue_fn) {
      (*lua_pushvalue_fn)(L, idx);
   }
}

extern "C" void lua_remove(lua_State *L, int idx)
{
   ASSERT(lua_remove_fn);
   if (lua_remove_fn) {
      (*lua_remove_fn)(L, idx);
   }
}

extern "C" void lua_insert(lua_State *L, int idx)
{
   ASSERT(lua_insert_fn);
   if (lua_insert_fn) {
      (*lua_insert_fn)(L, idx);
   }
}

extern "C" void lua_replace(lua_State *L, int idx)
{
   ASSERT(lua_replace_fn);
   if (lua_replace_fn) {
      (*lua_replace_fn)(L, idx);
   }
}

extern "C" int lua_checkstack(lua_State *L, int sz)
{
   ASSERT(lua_checkstack_fn);
   if (lua_checkstack_fn) {
      return (*lua_checkstack_fn)(L, sz);
   }
   return 0;
}

extern "C" void lua_xmove(lua_State *from, lua_State *to, int n)
{
   ASSERT(lua_xmove_fn);
   if (lua_xmove_fn) {
      (*lua_xmove_fn)(from, to, n);
   }
}

extern "C" int lua_isnumber(lua_State *L, int idx)
{
   ASSERT(lua_isnumber_fn);
   if (lua_isnumber_fn) {
      return (*lua_isnumber_fn)(L, idx);
   }
   return 0;
}

extern "C" int lua_isstring(lua_State *L, int idx)
{
   ASSERT(lua_isstring_fn);
   if (lua_isstring_fn) {
      return (*lua_isstring_fn)(L, idx);
   }
   return 0;
}

extern "C" int lua_iscfunction(lua_State *L, int idx)
{
   ASSERT(lua_iscfunction_fn);
   if (lua_iscfunction_fn) {
      return (*lua_iscfunction_fn)(L, idx);
   }
   return 0;
}

extern "C" int lua_isuserdata(lua_State *L, int idx)
{
   ASSERT(lua_isuserdata_fn);
   if (lua_isuserdata_fn) {
      return (*lua_isuserdata_fn)(L, idx);
   }
   return 0;
}

extern "C" int lua_type(lua_State *L, int idx)
{
   ASSERT(lua_type_fn);
   if (lua_type_fn) {
      return (*lua_type_fn)(L, idx);
   }
   return 0;
}

extern "C" const char     * lua_typename(lua_State *L, int tp)
{
   ASSERT(lua_typename_fn);
   if (lua_typename_fn) {
      return (*lua_typename_fn)(L, tp);
   }
   return NULL;
}

extern "C" int lua_equal(lua_State *L, int idx1, int idx2)
{
   ASSERT(lua_equal_fn);
   if (lua_equal_fn) {
      return (*lua_equal_fn)(L, idx1, idx2);
   }
   return 0;
}

extern "C" int lua_rawequal(lua_State *L, int idx1, int idx2)
{
   ASSERT(lua_rawequal_fn);
   if (lua_rawequal_fn) {
      return (*lua_rawequal_fn)(L, idx1, idx2);
   }
   return 0;
}

extern "C" int lua_lessthan(lua_State *L, int idx1, int idx2)
{
   ASSERT(lua_lessthan_fn);
   if (lua_lessthan_fn) {
      return (*lua_lessthan_fn)(L, idx1, idx2);
   }
   return 0;
}

extern "C" lua_Number lua_tonumber(lua_State *L, int idx)
{
   ASSERT(lua_tonumber_fn);
   if (lua_tonumber_fn) {
      return (*lua_tonumber_fn)(L, idx);
   }
   return NULL;
}

extern "C" lua_Integer lua_tointeger(lua_State *L, int idx)
{
   ASSERT(lua_tointeger_fn);
   if (lua_tointeger_fn) {
      return (*lua_tointeger_fn)(L, idx);
   }
   return NULL;
}

extern "C" int lua_toboolean(lua_State *L, int idx)
{
   ASSERT(lua_toboolean_fn);
   if (lua_toboolean_fn) {
      return (*lua_toboolean_fn)(L, idx);
   }
   return 0;
}

extern "C" const char     * lua_tolstring(lua_State *L, int idx, size_t *len)
{
   ASSERT(lua_tolstring_fn);
   if (lua_tolstring_fn) {
      return (*lua_tolstring_fn)(L, idx, len);
   }
   return NULL;
}

extern "C" size_t lua_objlen(lua_State *L, int idx)
{
   ASSERT(lua_objlen_fn);
   if (lua_objlen_fn) {
      return (*lua_objlen_fn)(L, idx);
   }
   return NULL;
}

extern "C" lua_CFunction lua_tocfunction(lua_State *L, int idx)
{
   ASSERT(lua_tocfunction_fn);
   if (lua_tocfunction_fn) {
      return (*lua_tocfunction_fn)(L, idx);
   }
   return NULL;
}

extern "C" void          * lua_touserdata(lua_State *L, int idx)
{
   ASSERT(lua_touserdata_fn);
   if (lua_touserdata_fn) {
      return (*lua_touserdata_fn)(L, idx);
   }
   return NULL;
}

extern "C" lua_State      * lua_tothread(lua_State *L, int idx)
{
   ASSERT(lua_tothread_fn);
   if (lua_tothread_fn) {
      return (*lua_tothread_fn)(L, idx);
   }
   return NULL;
}

extern "C" const void     * lua_topointer(lua_State *L, int idx)
{
   ASSERT(lua_topointer_fn);
   if (lua_topointer_fn) {
      return (*lua_topointer_fn)(L, idx);
   }
   return NULL;
}

extern "C" void lua_pushnil(lua_State *L)
{
   ASSERT(lua_pushnil_fn);
   if (lua_pushnil_fn) {
      (*lua_pushnil_fn)(L);
   }
}

extern "C" void lua_pushnumber(lua_State *L, lua_Number n)
{
   ASSERT(lua_pushnumber_fn);
   if (lua_pushnumber_fn) {
      (*lua_pushnumber_fn)(L, n);
   }
}

extern "C" void lua_pushinteger(lua_State *L, lua_Integer n)
{
   ASSERT(lua_pushinteger_fn);
   if (lua_pushinteger_fn) {
      (*lua_pushinteger_fn)(L, n);
   }
}

extern "C" void lua_pushlstring(lua_State *L, const char *s, size_t l)
{
   ASSERT(lua_pushlstring_fn);
   if (lua_pushlstring_fn) {
      (*lua_pushlstring_fn)(L, s, l);
   }
}

extern "C" void lua_pushstring(lua_State *L, const char *s)
{
   ASSERT(lua_pushstring_fn);
   if (lua_pushstring_fn) {
      (*lua_pushstring_fn)(L, s);
   }
}

extern "C" const char * lua_pushvfstring(lua_State *L, const char *fmt, va_list argp)
{
   ASSERT(lua_pushvfstring_fn);
   if (lua_pushvfstring_fn) {
      return (*lua_pushvfstring_fn)(L, fmt, argp);
   }
   return NULL;
}

extern "C" void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n)
{
   ASSERT(lua_pushcclosure_fn);
   if (lua_pushcclosure_fn) {
      (*lua_pushcclosure_fn)(L, fn, n);
   }
}

extern "C" void lua_pushboolean(lua_State *L, int b)
{
   ASSERT(lua_pushboolean_fn);
   if (lua_pushboolean_fn) {
      (*lua_pushboolean_fn)(L, b);
   }
}

extern "C" void lua_pushlightuserdata(lua_State *L, void *p)
{
   ASSERT(lua_pushlightuserdata_fn);
   if (lua_pushlightuserdata_fn) {
      (*lua_pushlightuserdata_fn)(L, p);
   }
}

extern "C" int lua_pushthread(lua_State *L)
{
   ASSERT(lua_pushthread_fn);
   if (lua_pushthread_fn) {
      return (*lua_pushthread_fn)(L);
   }
   return 0;
}

extern "C" void lua_gettable(lua_State *L, int idx)
{
   ASSERT(lua_gettable_fn);
   if (lua_gettable_fn) {
      (*lua_gettable_fn)(L, idx);
   }
}

extern "C" void lua_getfield(lua_State *L, int idx, const char *k)
{
   ASSERT(lua_getfield_fn);
   if (lua_getfield_fn) {
      (*lua_getfield_fn)(L, idx, k);
   }
}

extern "C" void lua_rawget(lua_State *L, int idx)
{
   ASSERT(lua_rawget_fn);
   if (lua_rawget_fn) {
      (*lua_rawget_fn)(L, idx);
   }
}

extern "C" void lua_rawgeti(lua_State *L, int idx, int n)
{
   ASSERT(lua_rawgeti_fn);
   if (lua_rawgeti_fn) {
      (*lua_rawgeti_fn)(L, idx, n);
   }
}

extern "C" void lua_createtable(lua_State *L, int narr, int nrec)
{
   ASSERT(lua_createtable_fn);
   if (lua_createtable_fn) {
      (*lua_createtable_fn)(L, narr, nrec);
   }
}

extern "C" void * lua_newuserdata(lua_State *L, size_t sz)
{
   ASSERT(lua_newuserdata_fn);
   if (lua_newuserdata_fn) {
      return (*lua_newuserdata_fn)(L, sz);
   }
   return NULL;
}

extern "C" int lua_getmetatable(lua_State *L, int objindex)
{
   ASSERT(lua_getmetatable_fn);
   if (lua_getmetatable_fn) {
      return (*lua_getmetatable_fn)(L, objindex);
   }
   return 0;
}

extern "C" void lua_getfenv(lua_State *L, int idx)
{
   ASSERT(lua_getfenv_fn);
   if (lua_getfenv_fn) {
      (*lua_getfenv_fn)(L, idx);
   }
}

extern "C" void lua_settable(lua_State *L, int idx)
{
   ASSERT(lua_settable_fn);
   if (lua_settable_fn) {
      (*lua_settable_fn)(L, idx);
   }
}

extern "C" void lua_setfield(lua_State *L, int idx, const char *k)
{
   ASSERT(lua_setfield_fn);
   if (lua_setfield_fn) {
      (*lua_setfield_fn)(L, idx, k);
   }
}

extern "C" void lua_rawset(lua_State *L, int idx)
{
   ASSERT(lua_rawset_fn);
   if (lua_rawset_fn) {
      (*lua_rawset_fn)(L, idx);
   }
}

extern "C" void lua_rawseti(lua_State *L, int idx, int n)
{
   ASSERT(lua_rawseti_fn);
   if (lua_rawseti_fn) {
      (*lua_rawseti_fn)(L, idx, n);
   }
}

extern "C" int lua_setmetatable(lua_State *L, int objindex)
{
   ASSERT(lua_setmetatable_fn);
   if (lua_setmetatable_fn) {
      return (*lua_setmetatable_fn)(L, objindex);
   }
   return 0;
}

extern "C" int lua_setfenv(lua_State *L, int idx)
{
   ASSERT(lua_setfenv_fn);
   if (lua_setfenv_fn) {
      return (*lua_setfenv_fn)(L, idx);
   }
   return 0;
}

extern "C" void lua_call(lua_State *L, int nargs, int nresults)
{
   ASSERT(lua_call_fn);
   if (lua_call_fn) {
      (*lua_call_fn)(L, nargs, nresults);
   }
}

extern "C" int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc)
{
   ASSERT(lua_pcall_fn);
   if (lua_pcall_fn) {
      return (*lua_pcall_fn)(L, nargs, nresults, errfunc);
   }
   return 0;
}

extern "C" int lua_cpcall(lua_State *L, lua_CFunction func, void *ud)
{
   ASSERT(lua_cpcall_fn);
   if (lua_cpcall_fn) {
      return (*lua_cpcall_fn)(L, func, ud);
   }
   return 0;
}

extern "C" int lua_load(lua_State *L, lua_Reader reader, void *dt, const char *chunkname)
{
   ASSERT(lua_load_fn);
   if (lua_load_fn) {
      return (*lua_load_fn)(L, reader, dt, chunkname);
   }
   return 0;
}

extern "C" int lua_dump(lua_State *L, lua_Writer writer, void *data)
{
   ASSERT(lua_dump_fn);
   if (lua_dump_fn) {
      return (*lua_dump_fn)(L, writer, data);
   }
   return 0;
}

extern "C" int lua_yield(lua_State *L, int nresults)
{
   ASSERT(lua_yield_fn);
   if (lua_yield_fn) {
      return (*lua_yield_fn)(L, nresults);
   }
   return 0;
}

extern "C" int lua_resume(lua_State *L, int narg)
{
   ASSERT(lua_resume_fn);
   if (lua_resume_fn) {
      return (*lua_resume_fn)(L, narg);
   }
   return 0;
}

extern "C" int lua_status(lua_State *L)
{
   ASSERT(lua_status_fn);
   if (lua_status_fn) {
      return (*lua_status_fn)(L);
   }
   return 0;
}

extern "C" int lua_error(lua_State *L)
{
   ASSERT(lua_error_fn);
   if (lua_error_fn) {
      return (*lua_error_fn)(L);
   }
   return 0;
}

extern "C" int lua_gc(lua_State *L, int what, int data)
{
   ASSERT(lua_gc_fn);
   if (lua_gc_fn) {
      return (*lua_gc_fn)(L, what, data);
   }
   return 0;
}

extern "C" int lua_next(lua_State *L, int idx)
{
   ASSERT(lua_next_fn);
   if (lua_next_fn) {
      return (*lua_next_fn)(L, idx);
   }
   return 0;
}

extern "C" void lua_concat(lua_State *L, int n)
{
   ASSERT(lua_concat_fn);
   if (lua_concat_fn) {
      (*lua_concat_fn)(L, n);
   }
}

extern "C" lua_Alloc lua_getallocf(lua_State *L, void **ud)
{
   ASSERT(lua_getallocf_fn);
   if (lua_getallocf_fn) {
      return (*lua_getallocf_fn)(L, ud);
   }
   return NULL;
}

extern "C" void lua_setallocf(lua_State *L, lua_Alloc f, void *ud)
{
   ASSERT(lua_setallocf_fn);
   if (lua_setallocf_fn) {
      (*lua_setallocf_fn)(L, f, ud);
   }
}

extern "C" lua_Alloc2 lua_getalloc2f(lua_State *L, void **ud)
{
   ASSERT(lua_getalloc2f_fn);
   if (lua_getalloc2f_fn) {
      return (*lua_getalloc2f_fn)(L, ud);
   }
   return NULL;
}

extern "C" void lua_setalloc2f(lua_State *L, lua_Alloc2 f, void *ud)
{
   ASSERT(lua_setalloc2f_fn);
   if (!lua_setalloc2f_fn) {
      LOG(lua.code, 1) << "could not find lua_setalloc2f entry point in lua dll.  is the jit on?";
      return;
   }
   (*lua_setalloc2f_fn)(L, f, ud);
}

extern "C" void lua_setlevel(lua_State *from, lua_State *to)
{
   ASSERT(lua_setlevel_fn);
   if (lua_setlevel_fn) {
      (*lua_setlevel_fn)(from, to);
   }
}

extern "C" int lua_getstack(lua_State *L, int level, lua_Debug *ar)
{
   ASSERT(lua_getstack_fn);
   if (lua_getstack_fn) {
      return (*lua_getstack_fn)(L, level, ar);
   }
   return 0;
}

extern "C" int lua_getinfo(lua_State *L, const char *what, lua_Debug *ar)
{
   ASSERT(lua_getinfo_fn);
   if (lua_getinfo_fn) {
      return (*lua_getinfo_fn)(L, what, ar);
   }
   return 0;
}

extern "C" const char * lua_getlocal(lua_State *L, const lua_Debug *ar, int n)
{
   ASSERT(lua_getlocal_fn);
   if (lua_getlocal_fn) {
      return (*lua_getlocal_fn)(L, ar, n);
   }
   return NULL;
}

extern "C" const char * lua_setlocal(lua_State *L, const lua_Debug *ar, int n)
{
   ASSERT(lua_setlocal_fn);
   if (lua_setlocal_fn) {
      return (*lua_setlocal_fn)(L, ar, n);
   }
   return NULL;
}

extern "C" const char * lua_getupvalue(lua_State *L, int funcindex, int n)
{
   ASSERT(lua_getupvalue_fn);
   if (lua_getupvalue_fn) {
      return (*lua_getupvalue_fn)(L, funcindex, n);
   }
   return NULL;
}

extern "C" const char * lua_setupvalue(lua_State *L, int funcindex, int n)
{
   ASSERT(lua_setupvalue_fn);
   if (lua_setupvalue_fn) {
      return (*lua_setupvalue_fn)(L, funcindex, n);
   }
   return NULL;
}

extern "C" int lua_sethook(lua_State *L, lua_Hook func, int mask, int count)
{
   ASSERT(lua_sethook_fn);
   if (lua_sethook_fn) {
      return (*lua_sethook_fn)(L, func, mask, count);
   }
   return 0;
}

extern "C" lua_Hook lua_gethook(lua_State *L)
{
   ASSERT(lua_gethook_fn);
   if (lua_gethook_fn) {
      return (*lua_gethook_fn)(L);
   }
   return NULL;
}

extern "C" int lua_gethookmask(lua_State *L)
{
   ASSERT(lua_gethookmask_fn);
   if (lua_gethookmask_fn) {
      return (*lua_gethookmask_fn)(L);
   }
   return 0;
}

extern "C" int lua_gethookcount(lua_State *L)
{
   ASSERT(lua_gethookcount_fn);
   if (lua_gethookcount_fn) {
      return (*lua_gethookcount_fn)(L);
   }
   return 0;
}

extern "C" void luaI_openlib(lua_State *L, const char *libname, const luaL_Reg *l, int nup)
{
   ASSERT(luaI_openlib_fn);
   if (luaI_openlib_fn) {
      (*luaI_openlib_fn)(L, libname, l, nup);
   }
}

extern "C" void luaL_register(lua_State *L, const char *libname, const luaL_Reg *l)
{
   ASSERT(luaL_register_fn);
   if (luaL_register_fn) {
      (*luaL_register_fn)(L, libname, l);
   }
}

extern "C" int luaL_getmetafield(lua_State *L, int obj, const char *e)
{
   ASSERT(luaL_getmetafield_fn);
   if (luaL_getmetafield_fn) {
      return (*luaL_getmetafield_fn)(L, obj, e);
   }
   return 0;
}

extern "C" int luaL_callmeta(lua_State *L, int obj, const char *e)
{
   ASSERT(luaL_callmeta_fn);
   if (luaL_callmeta_fn) {
      return (*luaL_callmeta_fn)(L, obj, e);
   }
   return 0;
}

extern "C" int luaL_typerror(lua_State *L, int narg, const char *tname)
{
   ASSERT(luaL_typerror_fn);
   if (luaL_typerror_fn) {
      return (*luaL_typerror_fn)(L, narg, tname);
   }
   return 0;
}

extern "C" int luaL_argerror(lua_State *L, int numarg, const char *extramsg)
{
   ASSERT(luaL_argerror_fn);
   if (luaL_argerror_fn) {
      return (*luaL_argerror_fn)(L, numarg, extramsg);
   }
   return 0;
}

extern "C" const char * luaL_checklstring(lua_State *L, int numArg, size_t *l)
{
   ASSERT(luaL_checklstring_fn);
   if (luaL_checklstring_fn) {
      return (*luaL_checklstring_fn)(L, numArg, l);
   }
   return NULL;
}

extern "C" const char * luaL_optlstring(lua_State *L, int numArg, const char *def, size_t *l)
{
   ASSERT(luaL_optlstring_fn);
   if (luaL_optlstring_fn) {
      return (*luaL_optlstring_fn)(L, numArg, def, l);
   }
   return NULL;
}

extern "C" lua_Number luaL_checknumber(lua_State *L, int numArg)
{
   ASSERT(luaL_checknumber_fn);
   if (luaL_checknumber_fn) {
      return (*luaL_checknumber_fn)(L, numArg);
   }
   return NULL;
}

extern "C" lua_Number luaL_optnumber(lua_State *L, int nArg, lua_Number def)
{
   ASSERT(luaL_optnumber_fn);
   if (luaL_optnumber_fn) {
      return (*luaL_optnumber_fn)(L, nArg, def);
   }
   return NULL;
}

extern "C" lua_Integer luaL_checkinteger(lua_State *L, int numArg)
{
   ASSERT(luaL_checkinteger_fn);
   if (luaL_checkinteger_fn) {
      return (*luaL_checkinteger_fn)(L, numArg);
   }
   return NULL;
}

extern "C" lua_Integer luaL_optinteger(lua_State *L, int nArg, lua_Integer def)
{
   ASSERT(luaL_optinteger_fn);
   if (luaL_optinteger_fn) {
      return (*luaL_optinteger_fn)(L, nArg, def);
   }
   return NULL;
}

extern "C" void luaL_checkstack(lua_State *L, int sz, const char *msg)
{
   ASSERT(luaL_checkstack_fn);
   if (luaL_checkstack_fn) {
      (*luaL_checkstack_fn)(L, sz, msg);
   }
}

extern "C" void luaL_checktype(lua_State *L, int narg, int t)
{
   ASSERT(luaL_checktype_fn);
   if (luaL_checktype_fn) {
      (*luaL_checktype_fn)(L, narg, t);
   }
}

extern "C" void luaL_checkany(lua_State *L, int narg)
{
   ASSERT(luaL_checkany_fn);
   if (luaL_checkany_fn) {
      (*luaL_checkany_fn)(L, narg);
   }
}

extern "C" int luaL_newmetatable(lua_State *L, const char *tname)
{
   ASSERT(luaL_newmetatable_fn);
   if (luaL_newmetatable_fn) {
      return (*luaL_newmetatable_fn)(L, tname);
   }
   return 0;
}

extern "C" void * luaL_checkudata(lua_State *L, int ud, const char *tname)
{
   ASSERT(luaL_checkudata_fn);
   if (luaL_checkudata_fn) {
      return (*luaL_checkudata_fn)(L, ud, tname);
   }
   return NULL;
}

extern "C" void luaL_where(lua_State *L, int lvl)
{
   ASSERT(luaL_where_fn);
   if (luaL_where_fn) {
      (*luaL_where_fn)(L, lvl);
   }
}

extern "C" int luaL_checkoption(lua_State *L, int narg, const char *def, const char *const lst[])
{
   ASSERT(luaL_checkoption_fn);
   if (luaL_checkoption_fn) {
      return (*luaL_checkoption_fn)(L, narg, def, lst);
   }
   return 0;
}

extern "C" int luaL_ref(lua_State *L, int t)
{
   ASSERT(luaL_ref_fn);
   if (luaL_ref_fn) {
      return (*luaL_ref_fn)(L, t);
   }
   return 0;
}

extern "C" void luaL_unref(lua_State *L, int t, int ref)
{
   ASSERT(luaL_unref_fn);
   if (luaL_unref_fn) {
      (*luaL_unref_fn)(L, t, ref);
   }
}

extern "C" int luaL_loadfile(lua_State *L, const char *filename)
{
   ASSERT(luaL_loadfile_fn);
   if (luaL_loadfile_fn) {
      return (*luaL_loadfile_fn)(L, filename);
   }
   return 0;
}

extern "C" int luaL_loadbuffer(lua_State *L, const char *buff, size_t sz, const char *name)
{
   ASSERT(luaL_loadbuffer_fn);
   if (luaL_loadbuffer_fn) {
      return (*luaL_loadbuffer_fn)(L, buff, sz, name);
   }
   return 0;
}

extern "C" int luaL_loadstring(lua_State *L, const char *s)
{
   ASSERT(luaL_loadstring_fn);
   if (luaL_loadstring_fn) {
      return (*luaL_loadstring_fn)(L, s);
   }
   return 0;
}

/* could not match params void parsing luaL_newstate */
extern "C" lua_State * luaL_newstate(void)
{
   ASSERT(luaL_newstate_fn);
   if (luaL_newstate_fn) {
      return (*luaL_newstate_fn)();
   }
   return NULL;
}

extern "C" const char * luaL_gsub(lua_State *L, const char *s, const char *p, const char *r)
{
   ASSERT(luaL_gsub_fn);
   if (luaL_gsub_fn) {
      return (*luaL_gsub_fn)(L, s, p, r);
   }
   return NULL;
}

extern "C" const char * luaL_findtable(lua_State *L, int idx, const char *fname, int szhint)
{
   ASSERT(luaL_findtable_fn);
   if (luaL_findtable_fn) {
      return (*luaL_findtable_fn)(L, idx, fname, szhint);
   }
   return NULL;
}

extern "C" void luaL_openlibs(lua_State *L)
{
   ASSERT(luaL_openlibs_fn);
   if (luaL_openlibs_fn) {
      (*luaL_openlibs_fn)(L);
   }
}

extern "C" const char * lua_pushfstring(lua_State *L, const char *fmt, ...)
{
   ASSERT(lua_pushvfstring_fn);
   if (lua_pushvfstring_fn) {
      va_list argp;
      va_start(argp, fmt);
      const char *result = (*lua_pushvfstring_fn)(L, fmt, argp);
      va_end(argp);
   }
   return NULL;
}

extern "C" int luaL_error(lua_State *L, const char *fmt, ...)
{
   if (luaL_error_fn) {
      // this is SO ANNOYING!  get rid of dynamic loading and always use lua jit if
      // we can sort out the decoda issues...
      char buffer[4096];
      va_list args;
      va_start(args, fmt);
      vsprintf(buffer, fmt, args);
      va_end(args);
      return (*luaL_error_fn)(L, buffer);
   }
   return 0;
}
