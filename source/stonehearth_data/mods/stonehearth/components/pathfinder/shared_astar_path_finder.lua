NEXT_ID = 0

-- class SharedAStarPathFinder
--
-- used to share a bfs pathfinder instance between multiple requestors.
--
local SharedAStarPathFinder = class()

-- create a shared bfs pathfinder.
--
--    @param entity - the source entity
--    @param start_location - the location where `entity` is starting from
--    @param on_destroy_cb - the callback to call when the last solved cb has been removed
--
function SharedAStarPathFinder:__init(entity, start_location, on_destroy_cb)
   NEXT_ID = NEXT_ID + 1
   self._id = NEXT_ID
   self._entity = entity
   self._start_location = start_location
   self._on_destroy_cb = on_destroy_cb

   self._log = radiant.log.create_logger('astar_path_finder')
                          :set_entity(self._entity)
   self._destinations = {}

   self._log:info("created")
end


-- destroys the bfs pathfinder.   the pathfinder is destroyed when the last solution function
-- has been un-registered (i.e. when no one else needs it)
--
function SharedAStarPathFinder:_destroy()
   self._log:info("destroying")
   self:_stop_pathfinder()
   self._on_destroy_cb()
end


-- registered the `solved_cb` to notify a client that a solution has been found.  this can be
-- called multiple time to register multiple clients.
--
--    @param solved_cb - a callback to call when a solution is found
--
function SharedAStarPathFinder:add_destination(dst, solved_cb)
   self:_start_pathfinder();

   self._log:info("adding %s to search (solved_cb:%s)", tostring(dst), tostring(solved_cb))
   local id = dst:get_id()
   local solved_cbs = self._destinations[id]
   if not solved_cbs then
      solved_cbs = {}
      self._destinations[id] = solved_cbs
      self._pathfinder:add_destination(dst)
   end
   solved_cbs[solved_cb] = true
end

-- unregisters a previously registered `solved_cb`.  whent he last callback has been unregistered
-- the bfs pathfinder will self destruct.
--
--    @param solved_cb - the callback to remove
--
function SharedAStarPathFinder:remove_destination(id, solved_cb)
   self._log:info("removing entity %d from search (solved_cb:%s)", id, tostring(solved_cb))
   local solved_cbs = self._destinations[id]
   if solved_cbs then
      solved_cbs[solved_cb] = nil
      if not next(solved_cbs) then
         self._log:detail('that was the last one. removing destination.')
         self._pathfinder:remove_destination(id)
         self._destinations[id] = nil
      end
   end
   if not next(self._destinations) then
      self._log:detail('that was the last destination.  stopping pathfinder.')
      self:_stop_pathfinder()
   end
end

-- starts the pathfinder.  the pathfinder gets started the first time a solved_cb is added
-- via :add_solved_cb
--
function SharedAStarPathFinder:_start_pathfinder()
   if not self._pathfinder then
      self._log:info("starting astar pathfinder from %s", self._start_location)   
      self._pathfinder = _radiant.sim.create_astar_path_finder(self._entity, string.format('shared astar from %s', tostring(self._start_location)))
                           :set_source(self._start_location)
                           :set_solved_cb(function(path)
                                 return self:_on_solved(path)
                              end)
                           :start()
                           
      self._log:info('created astar pathfinder id:%d name:%s for %s @ %s',
                     self._pathfinder:get_id(), self._pathfinder:get_name(),
                     self._entity, self._start_location)
   end
end

function SharedAStarPathFinder:_on_solved(path)
   local dst = path:get_destination()
   local id = dst:get_id()

   -- copy the solved cbs array, just in case we get called back in some crazy way
   local solved_cbs = self._destinations[id]
   local local_solved_cbs = {}
   for cb, _ in pairs(solved_cbs) do
      table.insert(local_solved_cbs, cb)
   end

   -- now run through each solved cb...
   for _, cb in ipairs(local_solved_cbs) do
      if solved_cbs[cb] then
         local finished = cb(path)
         if finished then
            self:remove_destination(id, cb)
         end
      end
   end

   -- mark the search as solved if we've run out of destinations to path to
   return not next(self._destinations)
end

-- stops the pathfinder.  the pathfinder is stopped during destruction, which happens right
-- when the last `solved_cb` is removed.
--
function SharedAStarPathFinder:_stop_pathfinder()
   if self._pathfinder then
      self._log:info("stopping pathfinder from %s", self._start_location)   
      self._pathfinder:destroy()
      self._pathfinder = nil
   end
end

return SharedAStarPathFinder
