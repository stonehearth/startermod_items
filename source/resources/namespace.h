#ifndef _RADIANT_RES_NAMESPACE_H
#define _RADIANT_RES_NAMESPACE_H

#define BEGIN_RADIANT_RES_NAMESPACE  namespace radiant { namespace res {
#define END_RADIANT_RES_NAMESPACE    } }

BEGIN_RADIANT_RES_NAMESPACE

class IModule;
class LuaFileMapper;
class ResourceCompiler;

namespace fbs {
   struct LuaFileIndex;
   struct LuaFileMapper;
};

DECLARE_SHARED_POINTER_TYPES(LuaFileMapper);
DECLARE_SHARED_POINTER_TYPES(ResourceCompiler)

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_NAMESPACE_H
