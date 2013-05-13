#ifndef _RADIANT_SIMULATION_SCRIPT_HOST_H
#define _RADIANT_SIMULATION_SCRIPT_HOST_H

#include "platform/utils.h"
#include "platform/random.h"
#include "om/om.h"
#include "om/entity.h"
#include "namespace.h"
#include "math3d.h"
#include "tesseract.pb.h"

#define CATCH_LUA_ERROR(x) \
   catch (luabind::cast_failed& e) { \
      std::ostringstream out; \
      LOG(WARNING) << "caught luabind::cast_failed."; \
      out << "lua error " << x << ": " << e.what() << " in call " << e.info().name(); \
      ScriptHost::GetInstance().OnError(out.str()); \
   } catch (luabind::error& e) { \
      std::ostringstream out; \
      LOG(WARNING) << "caught luabind::error."; \
      out << "lua error: " << x << " " << e.what() << ", " << lua_tostring(e.state(), -1) << std::endl; \
      ScriptHost::GetInstance().OnError(out.str()); \
   } catch (std::exception& e) { \
      std::ostringstream out; \
      LOG(WARNING) << "caught exception."; \
      out << "error: " << e.what() << std::endl; \
      ScriptHost::GetInstance().OnError(out.str()); \
   }

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Simulation;
class LuaMob;
class PathFinder;
class MultiPathFinder;
class FollowPath;
class GotoLocation;
class Path;

struct ScriptError
{
};


class ScriptHost {
public:
   ScriptHost(lua_State* L, std::string scriptRoot);
   ~ScriptHost();

   static ScriptHost& GetInstance();

   lua_State* GetInterpreter() const { return L_; }

   void SendMsg(om::EntityRef entity, std::string msg);
   void SendMsg(om::EntityRef entity, std::string msg, const luabind::object& arg0);

   void Call(std::string fn, luabind::object arg1);
   
   template <class T1, class T2>
   void CallFunction(T1 fn, T2 arg1) {
      try {
         call_function<void>(fn, arg1);
      } CATCH_LUA_ERROR("calling function...");
   }
   template <class T1, class T2, class T3>
   void CallFunction(T1 fn, T2 arg1, T3 arg2) {
      try {
         call_function<void>(fn, arg1, arg2);
      } CATCH_LUA_ERROR("calling function...");
   }

   std::string DoAction(const tesseract::protocol::DoAction& msg);
   void CreateNew();
   void Update(int interval, int& currentGameTime);
   void Idle(platform::timer &timer);
   om::EntityPtr CreateEntity(std::string kind);
   om::EntityPtr GetEntity(om::EntityId id) { auto i = entityMap_.find(id); return i == entityMap_.end() ? nullptr : i->second; }

private:
   static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize);

   om::GridPtr CreateGrid();
   om::EntityRef CreateEntityRef(std::string kind);
   luabind::object GetEntityRef(om::EntityId id);
   void InitEnvironment(std::string root);
   void LoadRecursive(std::string root, std::string directory);
   void LoadScript(std::string path);
   luabind::object ConvertArg(const Protocol::Selection& arg);

   void RegisterScenario(luabind::object name, luabind::object scenario);
   void ReportError(luabind::object error);
   luabind::object LookupResource(std::string name);
   void Log(std::string str);

private:

   void LoadGameScript();
   void DestroyEntity(std::weak_ptr<om::Entity>);

   std::shared_ptr<FollowPath> CreateFollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance);
   std::shared_ptr<GotoLocation> CreateGotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance);
   std::shared_ptr<GotoLocation> CreateGotoEntity(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance);
   std::shared_ptr<MultiPathFinder> CreateMultiPathFinder(std::string name);
   std::shared_ptr<PathFinder> CreatePathFinder(std::string name, om::EntityRef e);
   
public:
   void OnError(std::string description);
   void AssertFailed(std::string reason);
   void Unstick(om::EntityRef entity);
   void CallEnvironment(std::string method);

private:
   static ScriptHost* singleton_;

private:
   lua_State*           L_;
   luabind::object      game_;
   std::map<om::EntityId, om::EntityPtr>  entityMap_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_SCRIPT_HOST_H