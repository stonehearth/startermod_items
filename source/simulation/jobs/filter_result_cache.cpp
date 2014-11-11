#include "pch.h"
#include "filter_result_cache.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "dm/boxed_trace.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define BFS_LOG(level)   LOG(simulation.pathfinder.bfs, level) << "[ filter result cache ] "


/*
 * -- FilterResultCache::FilterResultCache
 *
 * Not so complicated, eh?
 *
 */
FilterResultCache::FilterResultCache()
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
bool FilterResultCache::ConsiderEntity(om::EntityPtr entity)
{
   if (!entity) {
      return false;
   }
   dm::ObjectId id = entity->GetObjectId();
   auto i = _results.find(id);
   if (i != _results.end()) {
      bool result = i->second.value;
      BFS_LOG(9) << "returning cached result (" << result << ") for " << *entity;
      return result;
   }
   
   bool result = _filterFn(entity);

   dm::TracePtr trace;
   om::MobPtr mob = entity->GetComponent<om::Mob>();
   if (mob) {
      trace = mob->TraceTransform("filter result cache", dm::OBJECT_MODEL_TRACES)
                                    ->OnModified([this, id]() {
                                       ClearCacheEntry(id);
                                    });
   }
   _results.insert(std::make_pair(id, Entry(result, trace)));
   BFS_LOG(9) << "cache miss!  adding new cache result result (" << result << ") for " << *entity;   
   return result;
}
