local SharedBfsPathFinder = require 'components.pathfinder.shared_bfs_path_finder'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local PathFinder = class()
local PATHFINDERS_CRASH_WHEN_GCED = {}

-- called to initialize the component on creation and loading.
--
function PathFinder:initialize(entity, json)
   self._entity = entity
   self._pathfinders = {}
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
function PathFinder:find_path_to_item_type(location, filter_fn, solved_cb)
   local pfkey = string.format('%s : %s', tostring(location), tostring(filter_fn))
   local pf = self._pathfinders[pfkey]

   if not pf then
      pf = SharedBfsPathFinder(self._entity, location, filter_fn, function()
            self._pathfinders[pfkey] = nil
         end)
      self._pathfinders[pfkey] = pf

      -- xxx: =(  remove this code as soon as we fix the GC problem with
      -- pathfinders!
      table.insert(PATHFINDERS_CRASH_WHEN_GCED, pf)
   end
   self.__saved_variables:mark_changed()

   pf:add_solved_cb(solved_cb)
   return radiant.lib.Destructor(function()
         pf:remove_solved_cb(solved_cb)
      end)
end

return PathFinder
