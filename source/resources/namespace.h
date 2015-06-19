#ifndef _RADIANT_RES_NAMESPACE_H
#define _RADIANT_RES_NAMESPACE_H

#define BEGIN_RADIANT_RES_NAMESPACE  namespace radiant { namespace res {
#define END_RADIANT_RES_NAMESPACE    } }

BEGIN_RADIANT_RES_NAMESPACE

class IModule;
class LuaFileMapper;
class ResourceCompiler;
class ResourceManager2;

namespace fbs {
   struct LuaFileIndex;
   struct LuaFileMapper;
};

struct LuaFunctionInfo {
   int startLine;
   int endLine;
   const char* functionName;
   LuaFunctionInfo() : functionName(""), startLine(0), endLine(0) {}
   LuaFunctionInfo(const char* f, int a, int z) : functionName(f), startLine(a), endLine(z) {}
};

DECLARE_SHARED_POINTER_TYPES(LuaFileMapper);
DECLARE_SHARED_POINTER_TYPES(ResourceCompiler)

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_NAMESPACE_H
