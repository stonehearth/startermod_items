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
   self._is_future = ai.CURRENT.future

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
         self._log:info('starting pathfinder after %.2fms delay', wait_time)
         self:_start_pathfinder(ai)
      end)
end

function FindPathToEntity:_start_pathfinder(ai)
   local on_success = function (path)
      self._search_exhausted_count = 0
      self._log:info('found solution: from(%s) to (%s) (count:%d)', path:get_start_point(), path, self._search_exhausted_count)
      ai:set_think_output({ path = path })
   end

   local on_exhausted = function()
      self._search_exhausted_count = self._search_exhausted_count + 1
      self._log:info('search exhausted (count:%d)!', self._search_exhausted_count)
   end

   self._pathfinder = self._entity:add_component('stonehearth:pathfinder')
                                    :find_path_to_entity(self._location, self._destination, on_success, on_exhausted)

   -- If we're not thinking ahead, then we're trying to find a path from where the AI is _right_ _now_.  If we
   -- move too far while thinking, restart the pathfinder.
   if not self._is_future then
      self._position_trace = radiant.entities.trace_grid_location(self._entity, 'path restart')
                                                :on_changed(function()
                                                   self:_on_position_changed(ai)
                                                end)
   end
end

function FindPathToEntity:_on_position_changed(ai)
   local cur_loc = radiant.entities.get_world_grid_location(self._entity)
   if cur_loc:distance_to(self._location) > 2 then
      self._log:info('restarting find_path_to_entity pathfinder (we\'ve gone too far!): started: %s, current: %s', self._location, cur_loc)

      self._location = cur_loc

      self:_cleanup()
      self:_start_pathfinder(ai)
   end
end

function FindPathToEntity:_cleanup()
   if self._pathfinder then
      self._pathfinder:destroy()
      self._pathfinder = nil
   end
   if self._position_trace then
      self._position_trace:destroy()
      self._position_trace = nil
   end
end

function FindPathToEntity:stop_thinking(ai, entity, args)
   self:_cleanup()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
   self._is_future = true
end

return FindPathToEntity
