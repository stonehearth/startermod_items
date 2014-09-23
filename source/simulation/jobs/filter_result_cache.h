#ifndef _RADIANT_SIMULATION_JOBS_FILTER_RESULT_CACHE_H
#define _RADIANT_SIMULATION_JOBS_FILTER_RESULT_CACHE_H

#include "simulation/namespace.h"
#include "dm/dm.h"
#include "om/om.h"
#include <memory>
#include <unordered_set>

BEGIN_RADIANT_SIMULATION_NAMESPACE

/*
 * -- FilterResultCache
 *
 * Caches the result of calling a filter function on an entity.  Use a filter entity
 * cache in circumstances where calling a filter function on an Entity will always (*)
 * produce the same result and you want to save the cost of the n-1 other, redundant
 * calls.
 *
 * The cache is invalided whenenver a client calls ClearCacheEntry or the entity
 * in question moves.  This tends to be quite useful (e.g. whether or not we should
 * stock an item depends on it's location (i.e. is it already in a stockpile?)).
 *
 */

class FilterResultCache : public std::enable_shared_from_this<FilterResultCache>
{
public:
   typedef std::function<bool(om::EntityPtr)> FilterFn;

public:
   FilterResultCache();

   FilterResultCachePtr SetFilterFn(FilterFn fn);
   FilterResultCachePtr ClearCacheEntry(dm::ObjectId id);
   bool ConsiderEntity(om::EntityPtr entity);

private:
   struct Entry {
      Entry(bool v, dm::TracePtr t) : value(v), trace(t) { }

      bool value;
      dm::TracePtr trace;
   };
private:
   typedef std::unordered_map<dm::ObjectId, Entry> FilterResults;

private:
   FilterFn       _filterFn;
   FilterResults  _results;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_FILTER_RESULT_CACHE_H

