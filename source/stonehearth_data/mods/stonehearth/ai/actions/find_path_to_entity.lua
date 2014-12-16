local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('pathfinder')
local rng = _radiant.csg.get_default_rng()

local MAX_BACKOFF = 30000    -- 30 seconds
local FindPathToEntity = class()

FindPathToEntity.name = 'find path to entity'
FindPathToEntity.does = 'stonehearth:find_path_to_entity'
FindPathToEntity.args = {
   destination = Entity,   -- entity to find a path to
}
FindPathToEntity.think_output = {
   path = Path,            -- the path to destination, from the current Entity
}
FindPathToEntity.version = 2
FindPathToEntity.priority = 1

function FindPathToEntity:__init()
   self._search_exhausted_count = 0
end

function FindPathToEntity:start_thinking(ai, entity, args)
   assert(not self._timer)
   assert(not self._pathfinder)
   self._log = ai:get_log()
   self._entity = entity
   self._location = ai.CURRENT.location
   self._destination = args.destination

   -- backoff..
   local wait_time = self._search_exhausted_count * 50
   if wait_time == 0 then
      self:_start_pathfinder(ai)
      return
   end
   wait_time = wait_time + rng:get_real(0, wait_time)
   wait_time = math.min(MAX_BACKOFF, wait_time)
   self._log:info('waiting %.2fms to start pathfinder', wait_time)
   self._timer = radiant.set_realtime_timer(wait_time, function()
         self._timer = nil
         self._log:info('starting pathfinder after %.2fs delay', wait_time)
         self:_start_pathfinder(ai)
      end)
end

function FindPathToEntity:_start_pathfinder(ai)
   local on_success = function (path)
      if ai.CURRENT.top_location and ai.CURRENT.top_location == ai.CURRENT.location then
         -- Assume it's the first move (wrong!) Check to see
         -- if we're too far from the start to do a proper path.
         local cur_loc = radiant.entities.get_world_grid_location(self._entity)
         if cur_loc:distance_to(ai.CURRENT.top_location) >= 2 then
            self._log:info('rejecting old solution (we\'ve gone too far!): started: %s, current: %s', ai.CURRENT.top_location, cur_loc)
            return false
         end
      end

      self._search_exhausted_count = 0
      self._log:info('found solution: %s', path)
      ai:set_think_output({ path = path })
      return true
   end
   local on_exhausted = function()
      self._search_exhausted_count = self._search_exhausted_count + 1
      self._log:info('search exhausted (count:%d)!', self._search_exhausted_count)
   end

   self._pathfinder = self._entity:add_component('stonehearth:pathfinder')
                                    :find_path_to_entity(self._location, self._destination, on_success, on_exhausted)
end

function FindPathToEntity:stop_thinking(ai, entity, args)
   if self._pathfinder then
      self._pathfinder:destroy()
      self._pathfinder = nil
   end
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

return FindPathToEntity
