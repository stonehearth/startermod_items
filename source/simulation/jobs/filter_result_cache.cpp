#include "pch.h"
#include "filter_result_cache.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/unit_info.ridl.h"
#include "dm/boxed_trace.h"
#include "lib/lua/script_host.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define BFS_LOG(level)   LOG(simulation.pathfinder.bfs, level) << "[ filter result cache ] "


/*
 * -- FilterResultCache::FilterResultCache
 *
 * Not so complicated, eh?
 *
 */
FilterResultCache::FilterResultCache(int flags) :
   _flags(flags)
{
}

/*
 * -- FilterResultCache::SetFilterFn
 *
 * Sets the filter function, whose result we shall cache!
 *
 */
FilterResultCachePtr FilterResultCache::SetFilterFn(FilterFn fn)
{
   _filterFn = fn;
   _results.clear();
   return shared_from_this();
}

FilterResultCachePtr FilterResultCache::SetLuaFilterFn(luabind::object unsafe_filter_fn)
{
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_filter_fn.interpreter());  
   luabind::object filter_fn = luabind::object(cb_thread, unsafe_filter_fn);

   SetFilterFn([filter_fn, cb_thread](om::EntityPtr e) mutable -> bool {
      MEASURE_TASK_TIME(Simulation::GetInstance().GetOverviewPerfTimeline(), "lua cb");
      try {
         LOG_CATEGORY(simulation.pathfinder.bfs, 5, "calling filter function on " << *e);
         luabind::object result = filter_fn(om::EntityRef(e));
         if (luabind::type(result) == LUA_TNIL) {
            return false;
         }
         if (luabind::type(result) == LUA_TBOOLEAN) {
            return luabind::object_cast<bool>(result);
         }
         return true;   // not nil or false is good enough for me!
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(cb_thread, e);
      }
      return false;
   });
   return shared_from_this();
}

/*
 * -- FilterResultCache::ClearCacheEntry
 *
 * Invalidate the cache for the specified entity.  This will cause the filter
 * function to get re-evaluated.
 *
 */
FilterResultCachePtr FilterResultCache::ClearCacheEntry(dm::ObjectId id)
{
   BFS_LOG(9) << "clearing cache entry for entity " << id;
   _results.erase(id);
   return shared_from_this();
}

/*
 * -- FilterResultCache::ConsiderEntity
 *
 * Return the result of calling the filter function on the entity.  Will only
 * actually call the filter function if it has never been called before or the entity has
 * moved since the last call.
 *
 */
bool FilterResultCache::ConsiderEntity(om::EntityPtr& entity)
{
   if (!entity) {
      return false;
   }
   
   om::MobPtr mob = entity->GetComponent<om::Mob>();
   if (!mob) {
      return false;
   }

   dm::ObjectId id = entity->GetObjectId();
   auto i = _results.find(id);

   bool useCachedResult = i != _results.end();
   if (!useCachedResult) {
      BFS_LOG(9) << "filter " << this << " cache miss for " << *entity << "!";
      i = _results.insert(std::make_pair(id, Entry())).first;
   }
   Entry& entry = i->second;

   if (_flags & INVALIDATE_ON_MOVE) {
      csg::Point3f& location = mob->GetLocation();
      if (location != entry.location) {
         useCachedResult = false;
         entry.location = location;
         BFS_LOG(9) << "filter " << this << " updating cached result location (" << location << ") for " << *entity;
      }
   }

   if (_flags & INVALIDATE_ON_PLAYER_ID_CHANGED) {
      core::StaticString playerId;
      om::UnitInfoPtr uip = entity->GetComponent<om::UnitInfo>();
      if (uip) {
         playerId = uip->GetPlayerId();
      }
      if (playerId != entry.playerId) {
         useCachedResult = false;
         entry.playerId = playerId;
         BFS_LOG(9) << "filter " << this << " updating cached result player id (" << playerId << ") for " << *entity;
      }
   }

   if (useCachedResult) {
      return entry.value;
   }
   bool result = _filterFn(entity);
   entry.value = result;
   BFS_LOG(9) << "filter " << this << " new cache result result (" << result << ") for " << *entity;
   return result;
}
