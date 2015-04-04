#ifndef _RADIANT_RES_LUA_FILE_MAPPER_H
#define _RADIANT_RES_LUA_FILE_MAPPER_H

#include <vector>
#include "namespace.h"
#include "lib/lua/bind.h"
#include "core/static_string.h"
#include "lua_file_mapper_generated.h"

BEGIN_RADIANT_RES_NAMESPACE

class LuaFileIndex {
public:
   void Load(fbs::LuaFileIndex const*);
   flatbuffers::Offset<fbs::LuaFileIndex> Save(flatbuffers::FlatBufferBuilder& fbb, core::StaticString filename) const;

   void AddFunction(core::StaticString fn, int min, int max);
   core::StaticString GetFunction(int line);
   std::string const& GetHash() const;
   void SetHash(std::string const& hash);

private:
   std::string                      _hash;
   std::vector<core::StaticString>  _lines;
};

class LuaFileMapper {
public:
   LuaFileMapper(lua::ScriptHost& sh);

   core::StaticString MapFileLineToFunction(core::StaticString file, int line);
   void IndexFile(core::StaticString filename, std::string const& contents);

private:
   void RequireFileMapper(lua::ScriptHost& sh);
   void Load();
   void Load(fbs::LuaFileMapper const* lfi);
   void Save() const;
   flatbuffers::Offset<fbs::LuaFileMapper> Save(flatbuffers::FlatBufferBuilder& fbb) const;

   LuaFileIndex& Find(core::StaticString filename, std::string const& contents, bool& needsIndexing);
   LuaFileIndex& Find(core::StaticString filename);

private:
   typedef std::unordered_map<core::StaticString, LuaFileIndex, core::StaticString::Hash> LuaFileMappingIndex;

private:
   lua::ScriptHost&     _sh;
   luabind::object      _flameGraphObj;
   luabind::object      _mapSourceFunctionsFn;
   LuaFileMappingIndex	_files;
   core::StaticString   _fbsPath;
};


END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_LUA_FILE_MAPPER_H