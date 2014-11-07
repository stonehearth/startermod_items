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
--    @param filter_result_cache - the filter to determine which items qualify as destinations
--    @param on_destroy_cb - the callback to call when the last solved cb has been removed
--
function SharedBfsPathFinder:__init(entity, start_location, filter_result_cache, on_destroy_cb)
   NEXT_ID = NEXT_ID + 1
   self._root_entity = radiant.entities.get_root_entity()

   self._id = NEXT_ID
   self._entity = entity
   self._start_location = start_location
   self._filter_result_cache = filter_result_cache
   self._on_destroy_cb = on_destroy_cb
   self._log = radiant.log.create_logger('bfs_path_finder')
                          :set_entity(self._entity)
   self._range = 512
   self._solved_cbs = {}
   self._solved_cb_count = 0

   self._log:info("created")

   self._reconsider_listener = radiant.events.listen(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self, self._reconsider_destination)
end


-- destroys the bfs pathfinder.   the pathfinder is destroyed when the last solution function
-- has been un-registered (i.e. when no one else needs it)
--
function SharedBfsPathFinder:_destroy()
   self._log:info("destroying")
   self:_stop_pathfinder()

   self._reconsider_listener:destroy()
   self._reconsider_listener = nil
   self._on_destroy_cb()
end

-- set the description to use when creating the cpp object
--
function SharedBfsPathFinder:set_description(description)
   self._description = description
   self._log:set_prefix(self._description)
end

-- get the description of what we're searching for
--
function SharedBfsPathFinder:get_description()
   return self._description
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
   self._solved_cb_count = self._solved_cb_count + 1
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
   self._solved_cb_count = self._solved_cb_count - 1
   if not next(self._solved_cbs) then
      self:_destroy()
   end
   return self._solved_cb_count
end

-- starts the pathfinder.  the pathfinder gets started the first time a solved_cb is added
-- via :add_solved_cb
--
function SharedBfsPathFinder:_start_pathfinder()
   local function solved(path)
      assert(not self._solution_path)
      local dst = path:get_destination()

      -- whoa, not so fast!  before we can pass this destination off to the pathfinders,
      -- make sure it's actually kosker.  "why isn't this done by the filter function?",
      -- you ask.  the filter function result for each entity is *heavily cached* by the
      -- FilterCacheResult object.  `_is_valid_destination` will check against certain
      -- transient conditions (e.g. can we get the ai lease?) which are inappropriate to
      -- cache
      if not self:_is_valid_destination(dst) then
         return false
      end
      self._solution_path = path
      self._log:debug('solved!')
      self:_call_solved_cbs(path)
      return true
   end

   self._log:info("starting bfs pathfinder from %s", self._start_location)   
   self._pathfinder = _radiant.sim.create_bfs_path_finder(self._entity, self._description, self._range)
                        :set_source(self._start_location)
                        :set_solved_cb(solved)
                        :set_filter_result_cache(self._filter_result_cache)
                        :start()
                        
   self._log:info('created bfs pathfinder id:%d name:%s for %s @ %s',
                  self._pathfinder:get_id(), self._pathfinder:get_name(),
                  self._entity, self._start_location)

end

-- stops the pathfinder.  the pathfinder is stopped during destruction, which happens right
-- when the last `solved_cb` is removed.
--
function SharedBfsPathFinder:_stop_pathfinder()
   if self._pathfinder then
      self._log:info("stopping pathfinder from %s", self._start_location)   
      self._pathfinder:destroy()
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
      self._log:info('%s not a valid destination.  already have solution path!', target)
      return false
   end

   if not self:_entity_visible_to_pathfinder(target) then
      self._log:info('%s not a valid destination.  made invisible to pathfinders.', target)
      return false
   end

   if not stonehearth.ai:can_acquire_ai_lease(target, self._entity) then
      self._log:info('%s not a valid destination.  could not acquire ai lease.', target)
      return false
   end

   self._log:spam('%s is a valid destination!', target)
   return true
end

function SharedBfsPathFinder:_entity_visible_to_pathfinder(target)
   local child = target

   while true do
      local parent = self:_get_parent_entity(child)

      if not parent then
         return false
      end

      if parent == self._root_entity then
         return true
      end

      local child_hidden = radiant.entities.get_entity_data(parent, 'stonehearth:hide_child_entities_from_pathfinder') or false
      if child_hidden then
         return false
      end

      child = parent
   end
end

function SharedBfsPathFinder:_get_parent_entity(target)
   local mob = target:get_component('mob')
   local parent = mob and mob:get_parent()
   return parent
end

function SharedBfsPathFinder:_reconsider_destination(target)
   if self._pathfinder then
      self._log:detail('reconsidering adding %s to pathfinder', target)
      self._pathfinder:reconsider_destination(target)
   end
end

return SharedBfsPathFinder
