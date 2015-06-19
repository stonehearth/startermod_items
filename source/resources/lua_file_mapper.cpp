#include <boost/filesystem.hpp>
#include "radiant.h"
#include "lua_file_mapper.h"
#include "lib/lua/script_host.h"
#include "flatbuffers/util.h"
#include "core/system.h"

using namespace ::radiant;
using namespace ::radiant::res;

extern std::string Checksum(std::string const& input);

LuaFileMapper::LuaFileMapper(lua::ScriptHost& sh) :
   _sh(sh)
{
   boost::filesystem::path path = core::System::GetInstance().GetTempDirectory() / "compiled_resources";
   boost::filesystem::create_directories(path);

   _fbsPath = (path / "lua_file_mapper.fb").string();

   Load();
   RequireFileMapper(sh);
}

void LuaFileMapper::RequireFileMapper(lua::ScriptHost& sh)
{
   try {
      _flameGraphObj = sh.Require("radiant.lib.compiler.file_mapper");
      _mapSourceFunctionsFn = _flameGraphObj["map_source_function"];
   } catch (std::exception const& e) {
      _sh.ReportCStackException(_sh.GetInterpreter(), e);
   }
}

void LuaFileMapper::Save() const
{
   flatbuffers::FlatBufferBuilder fbb;
   flatbuffers::Offset<fbs::LuaFileMapper> root = Save(fbb);
   fbb.Finish(root);

   flatbuffers::SaveFile(_fbsPath, (const char*)fbb.GetBufferPointer(), fbb.GetSize(), true);
}

flatbuffers::Offset<fbs::LuaFileMapper> LuaFileMapper::Save(flatbuffers::FlatBufferBuilder& fbb) const
{
   std::vector<flatbuffers::Offset<fbs::LuaFileIndex>> files;
   for (auto const &entry : _files) {
      files.push_back(entry.second.Save(fbb, entry.first));
   }
   return fbs::CreateLuaFileMapper(fbb, fbb.CreateVector(files));
}

void LuaFileMapper::Load()
{
   std::string buf;
   if (flatbuffers::LoadFile(_fbsPath, true, &buf)) {
      fbs::LuaFileMapper const* lfm = fbs::GetLuaFileMapper((void *)buf.c_str());
      Load(lfm);
   }
}

void LuaFileMapper::Load(fbs::LuaFileMapper const* lfi)
{
   _files.clear();
   for (auto const file : *lfi->files()) {
      _files[file->filename()->c_str()].Load(file);
   }
}

LuaFunctionInfo LuaFileMapper::MapFileLineToFunction(core::StaticString file, int line)
{
   return Find(file).GetFunction(line);
}

void LuaFileMapper::IndexFile(core::StaticString filename, std::string const& contents)
{
   bool needsIndexing = false;
   LuaFileIndex& fi = Find(filename, contents, needsIndexing);

   if (!needsIndexing) {
      return;
   }

   LOG(script_host, 0) << "indexing " << filename;

   try {
      luabind::object mapping = _mapSourceFunctionsFn(_flameGraphObj, contents, (const char*)filename);
      luabind::iterator i(mapping), end;
      for (; i != end; i++) {
         core::StaticString fn = luabind::object_cast<std::string>((*i)[1]);
         int min = luabind::object_cast<int>((*i)[2]);
         int max = luabind::object_cast<int>((*i)[3]);
         fi.AddFunction(fn, min, max);
      }
   } catch (std::exception const& e) {
      _sh.ReportCStackException(_mapSourceFunctionsFn.interpreter(), e);
   }
   Save();
}

LuaFileIndex& LuaFileMapper::Find(core::StaticString filename)
{
   LuaFileIndex& file = _files[filename];
   ASSERT(!file.GetHash().empty());
   return file;
}

LuaFileIndex& LuaFileMapper::Find(core::StaticString filename,
                                  std::string const& contents,
                                  bool& needsIndexing)
{
   std::string hash = Checksum(contents);
   LuaFileIndex& file = _files[filename];

   needsIndexing = file.GetHash() != hash;
   if (needsIndexing) {
      file.SetHash(hash);
   }
   return file;
}

void LuaFileIndex::Load(fbs::LuaFileIndex const* lffi)
{
   _lines.clear();
   _hash = lffi->contentsHash()->c_str();
   for (auto const function : *lffi->functions()) {
      AddFunction(function->name()->c_str(), function->min(), function->max());
   }
}

flatbuffers::Offset<fbs::LuaFileIndex> LuaFileIndex::Save(flatbuffers::FlatBufferBuilder& fbb, core::StaticString filename) const
{
   core::StaticString startFn;
   int startLine = 0, i = 0, c = (int)_lines.size();

   std::vector<flatbuffers::Offset<fbs::LuaFunctionRange>> functions;
   for (i = 0; i < c; i++) {
      core::StaticString fn = _lines[i];
      if (startFn != fn) {
         if (startFn != core::StaticString::Empty) {
            functions.emplace_back(fbs::CreateLuaFunctionRange(fbb, fbb.CreateString(startFn), startLine, i - 1));
         }
         startFn = fn;
         startLine = i;
      }
   }
   if (startFn != core::StaticString::Empty) {
      functions.emplace_back(fbs::CreateLuaFunctionRange(fbb, fbb.CreateString(startFn), startLine, i - 1));
   }

   return fbs::CreateLuaFileIndex(fbb, fbb.CreateString(filename), fbb.CreateString(_hash), fbb.CreateVector(functions));
}

void LuaFileIndex::AddFunction(core::StaticString fn, int min, int max)
{
   if ((int)_lines.size() <= max) {
      _lines.resize(max + 1);
   }
   for (int i = min; i <= max; i++) {
      _lines[i] = fn;
   }
}

LuaFunctionInfo LuaFileIndex::GetFunction(int line)
{
   ASSERT(line >= 0 && line < (int)_lines.size());
   LuaFunctionInfo result;

   int c = (int)_lines.size();

   result.functionName = _lines[line];
   result.startLine = 0;
   result.endLine = c - 1;

   while (result.startLine < c && _lines[result.startLine] != result.functionName) {
      ++result.startLine;
   }
   while (result.endLine > 0 && _lines[result.endLine] != result.functionName) {
      --result.endLine;
   }
   return result;
}

std::string const& LuaFileIndex::GetHash() const
{
   return _hash;
}

void LuaFileIndex::SetHash(std::string const& hash)
{
   _lines.clear();
   _hash = hash;
}
