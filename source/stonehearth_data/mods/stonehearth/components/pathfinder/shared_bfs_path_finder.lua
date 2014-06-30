NEXT_ID = 0

-- class SharedBfsPathFinder
--
-- used to share a bfs pathfinder instance between multiple requestors.
--
local SharedBfsPathFinder = class()

-- create a shared bfs pathfinder.
--
--    @param entity - the source entity
--    @param start_location - the location where `entity` is starting from
--    @param filter_fn - the filter to determine which items qualify as destinations
--    @param on_destroy_cb - the callback to call when the last solved cb has been removed
--
function SharedBfsPathFinder:__init(entity, start_location, filter_fn, on_destroy_cb)
   NEXT_ID = NEXT_ID + 1
   self._entity = entity
   self._start_location = start_location
   self._filter_fn = filter_fn
   self._description = string.format("id:%d", NEXT_ID)
   self._on_destroy_cb = on_destroy_cb
   self._log = radiant.log.create_logger('bfs_path_finder')
                          :set_prefix(self._description)
                          :set_entity(self._entity)
   self._range = 512
   self._solved_cbs = {}

   self._log:info("created")

   radiant.events.listen(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self, self._consider_destination)
end

-- destroys the bfs pathfinder.   the pathfinder is destroyed when the last solution function
-- has been un-registered (i.e. when no one else needs it)
--
function SharedBfsPathFinder:_destroy()
   self._log:info("destroying")
   self:_stop_pathfinder()
   radiant.events.unlisten(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self, self._consider_destination)
   self._on_destroy_cb()
end

-- registered the `solved_cb` to notify a client that a solution has been found.  this can be
-- called multiple time to register multiple clients.
--
--    @param solved_cb - a callback to call when a solution is found
--
function SharedBfsPathFinder:add_solved_cb(solved_cb)
   self._log:info("adding solved cb")
   if not next(self._solved_cbs) then
      self:_start_pathfinder()
   end
   self._solved_cbs[solved_cb] = solved_cb
   if self._solution_path then
      solved_cb(self._solution_path)
   end   
end

-- unregisters a previously registered `solved_cb`.  whent he last callback has been unregistered
-- the bfs pathfinder will self destruct.
--
--    @param solved_cb - the callback to remove
--
function SharedBfsPathFinder:remove_solved_cb(solved_cb)
   self._log:info("removing solved cb")
   self._solved_cbs[solved_cb] = nil
   if not next(self._solved_cbs) then
      self:_destroy()
   end
end

-- starts the pathfinder.  the pathfinder gets started the first time a solved_cb is added
-- via :add_solved_cb
--
function SharedBfsPathFinder:_start_pathfinder()
   local solved = function(path)
      self._solution_path = path
      self._solution_entity_id = path:get_destination():get_id()
      self._log:debug('solved!')
      self:_call_solved_cbs(path)
   end

   self._log:info("starting pathfinder from %s", self._start_location)   
   self._pathfinder = _radiant.sim.create_bfs_path_finder(self._entity, self._description, self._range)
                        :set_source(self._start_location)
                        :set_solved_cb(solved)
                        :set_filter_fn(function(target)
                              return self:_is_valid_destination(target)
                           end)
                        :start()
end

-- stops the pathfinder.  the pathfinder is stopped during destruction, which happens right
-- when the last `solved_cb` is removed.
--
function SharedBfsPathFinder:_stop_pathfinder()
   if self._pathfinder then
      self._log:info("stopping pathfinder from %s", self._start_location)   
      self._pathfinder:stop()
      self._pathfinder = nil
   end
end

-- call all registered solution callbacks with the specified `path`.
--
--    @param path - the solution path
--
function SharedBfsPathFinder:_call_solved_cbs(path)
   for solved_cb, _ in pairs(self._solved_cbs) do
      solved_cb(path)
   end
end

-- returns whether or not the destination is a valid one.
--
--    @param target - the Entity being considered as a destination
--
function SharedBfsPathFinder:_is_valid_destination(target)
   if self._solution_path then
      -- hmm.  we already have a solution.  what if this object can provide a better one?
      -- do we really want to invalidate our current solution on the *chance* that it
      -- could be?  let's not.  let's just go with what we've got!
      return false
   end

   if not self._filter_fn(target, self._entity) then
      return false
   end

   if not stonehearth.ai:can_acquire_ai_lease(target, self._entity) then
      self._log:debug('ignoring %s (cannot acquire ai lease)', target)
      return false
   end

   self._log:detail('entity %s is ok!', target)
   return true
end

function SharedBfsPathFinder:_consider_destination(target)
   if self._pathfinder then
      self._log:detail('considering adding %s to pathfinder', target)
      self._pathfinder:reconsider_destination(target)
   end
end

return SharedBfsPathFinder
