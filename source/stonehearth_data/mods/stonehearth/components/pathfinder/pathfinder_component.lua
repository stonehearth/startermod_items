local SharedBfsPathFinder = require 'components.pathfinder.shared_bfs_path_finder'
local SharedAStarPathFinder = require 'components.pathfinder.shared_astar_path_finder'
local FilterResultCache = _radiant.sim.FilterResultCache
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local PathFinder = class()
local FILTER_RESULT_CACHES = {}

-- called to initialize the component on creation and loading.
--
function PathFinder:initialize(entity, json)
   self._entity = entity
   self._bfs_pathfinders = {}
   self._astar_pathfinders = {}
   self._filter_caches = {}
end

-- find a path from `location` to `item`, calling `solved_cb` when finished.
--
function PathFinder:find_path_to_entity(location, item, solved_cb)
   local pfkey = string.format('%s', tostring(location))
   local pf = self._astar_pathfinders[pfkey]
   if not pf then
      pf = SharedAStarPathFinder(self._entity, location, function()
               self._astar_pathfinders[pfkey] = nil
            end)
      self._astar_pathfinders[pfkey] = pf
   end

   local id = item:get_id()
   pf:add_destination(item, solved_cb)
   return radiant.lib.Destructor(function()
         return pf:remove_destination(id, solved_cb)
      end)
end

-- find a path from `location` to any item which matches the `filter_fn`.  All callers
-- who pass the exact same `location` and `filter_fn` will get a handle to the same bfs pathfinder.
-- so if 20 callers are all looking for the same thing, we'll only create 1 pathfinder on the
-- backend, and all 20 will be notified when it finds a solution.
--
-- to take full advantage of the sharing, callers must take VERY SPECIAL care to ensure
-- they're passing in the exact same `filter_fn` instance WHENEVER possible.
--
-- call :destroy() on the returned object to cancel a search.
--
--    @param location - the location to start from
--    @param filter_fn - the filter to determine which items qualify as destinations
--    @param on_destroy_cb - the callback to call when the last solved cb has been removed
--
function PathFinder:find_path_to_entity_type(location, filter_fn, description, solved_cb)
   -- create a filter cache base on the filter function.  multiple pathfinders from different
   -- entity sources and different locations and *all* share the same filter cache!!!
   local entry = FILTER_RESULT_CACHES[filter_fn]
   if not entry then
      local cache = FilterResultCache()
      cache:set_filter_fn(filter_fn)

      -- whenever someone else advises the result of a filter function may have changed, invalidate
      -- the entry in our filter_fn cache.  the cache is shared with and outlives all shared
      -- bfs pathfinder instances.  this listener must be kept on the cache even if there are no
      -- bfs pathfinders currently to avoid missing triggers which affect future pathfinding (liek SH-86).
      local listener = radiant.events.listen(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', function(e)
            cache:clear_cache_entry(e:get_id())
         end)

      entry = {
         listener = listener,
         cache = cache,
      }

      FILTER_RESULT_CACHES[filter_fn] = entry
      -- xxx: these currently don't get reaped.  we could do it on a timer...  i think that's
      -- probably really easy, but requires more thought (what if there's a pathfinder currently
      -- using it?  that's *probably* ok, since it's a shared_ptr... hrm) -tony
   end

   -- now find a shared pathfinder which wants to search from this location using this
   -- filter function and add the `solved_cb` to it.
   local pfkey = string.format('%s : %s', tostring(location), tostring(filter_fn))
   local pf = self._bfs_pathfinders[pfkey]
   if not pf then
      pf = SharedBfsPathFinder(self._entity, location, entry.cache, function()
               self._bfs_pathfinders[pfkey] = nil
            end)
      pf:set_description(description)      
      self._bfs_pathfinders[pfkey] = pf
   end
   assert(pf:get_description() == description)

   pf:add_solved_cb(solved_cb)
   return radiant.lib.Destructor(function()
         return pf:remove_solved_cb(solved_cb)
      end)
end

return PathFinder
