#ifndef _RADIANT_SIMULATION_SCRIPT_HOST_H
#define _RADIANT_SIMULATION_SCRIPT_HOST_H

#include "platform/utils.h"
#include "platform/random.h"
#include "om/om.h"
#include "om/entity.h"
#include "namespace.h"
#include "math3d.h"
#include "tesseract.pb.h"
#include "radiant_json.h"
#include "resources/animation.h"

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
   ScriptHost(lua_State* L);
   ~ScriptHost();

   static ScriptHost& GetInstance();

   lua_State* GetInterpreter() const { return L_; }

   void SendMsg(om::EntityRef entity, std::string msg);
   void SendMsg(om::EntityRef entity, std::string msg, const luabind::object& arg0);

   void Call(luabind::object fn, luabind::object arg1);
   luabind::object LuaRequire(std::string name);
   
   std::string DoAction(const tesseract::protocol::DoAction& msg);
   void CreateNew();
   void Update(int interval, int& currentGameTime);
   void CallGameHook(std::string const& stage);
   void Idle(platform::timer &timer);
   om::EntityRef CreateEntity();
   om::EntityRef GetEntity(om::EntityId id);

private:
   static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize);

   om::GridPtr CreateGrid();
   void InitEnvironment();
   void LoadRecursive(std::string root, std::string directory);
   luabind::object LoadScript(std::string path);
   luabind::object ConvertArg(const Protocol::Selection& arg);

   void RegisterScenario(luabind::object name, luabind::object scenario);
   void ReportError(luabind::object error);
   json::ConstJsonObject LoadJson(std::string uri);
   json::ConstJsonObject LoadManifest(std::string uri);
   resources::AnimationPtr LoadAnimation(std::string uri);
   void Log(std::string str);

private:

   void DestroyEntity(std::weak_ptr<om::Entity>);

   std::shared_ptr<FollowPath> CreateFollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance);
   std::shared_ptr<GotoLocation> CreateGotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance);
   std::shared_ptr<GotoLocation> CreateGotoEntity(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance);
   std::shared_ptr<MultiPathFinder> CreateMultiPathFinder(std::string name);
   std::shared_ptr<PathFinder> CreatePathFinder(std::string name, om::EntityRef e, luabind::object solved, luabind::object dst_filter);
   
public:
   void OnError(std::string description);
   void AssertFailed(std::string reason);
   void Unstick(om::EntityRef entity);
   void CallEnvironment(std::string method);

private:
   static ScriptHost* singleton_;

private:
   lua_State*           L_;
   lua_State*           cb_thread_;
   luabind::object      api_;
   luabind::object      game_;
   luabind::object      game_ctor_;
   std::map<std::string, luabind::object>    required_;
   std::map<om::EntityId, om::EntityPtr>     entityMap_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_SCRIPT_HOST_H