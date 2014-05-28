local log = radiant.log.create_logger('pathfinder')

local OptimizedPathfinder = class()

function OptimizedPathfinder:__init(entity, destination, success_cb, failure_cb)
   self._entity = entity
   self._destination = destination
   self._success_cb = success_cb
   self._failure_cb = failure_cb
   self._start_location = nil
end

function OptimizedPathfinder:set_start_location(start_location)
   self._start_location = start_location
   return self
end

function OptimizedPathfinder:start()
   if not self._entity or not self._entity:is_valid() then
      self:_on_failure('entity invalid')
      return
   end

   if not self._destination or not self._destination:is_valid() then
      self:_on_failure('destination invalid')
      return
   end

   if not self._start_location then
      self._start_location = radiant.entities.get_world_grid_location(self._entity)
   end

   if log:is_enabled(radiant.log.DEBUG) then
      local destination_location = radiant.entities.get_world_grid_location(self._destination)
      log:debug('finding path from CURRENT.location %s to %s (@ %s)',
                tostring(self._start_location), tostring(self._destination), tostring(destination_location))
   end

   local direct_path_finder = _radiant.sim.create_direct_path_finder(self._entity)
                                 :set_start_location(self._start_location)
                                 :set_destination_entity(self._destination)

   local path = direct_path_finder:get_path()

   if path then
      self:_on_success('direct path found', path)
      return
   end

   log:detail('no direct path found.  starting a* pathfinder.')
   
   self._trace = radiant.entities.trace_location(self._destination, 'OptimizedPathfinder')
      :on_changed(function()
         self:_on_failure('destination changed')
      end)
      :on_destroyed(function()
         self:_on_failure('destination destroyed')
      end)

   local solved = function(path)
      self:_on_success('a* path found', path)
   end

   self._pathfinder = _radiant.sim.create_astar_path_finder(self._entity, 'find path to entity')
                         :set_source(self._start_location)
                         :add_destination(self._destination)
                         :set_solved_cb(solved)
                         :start()

   return self
end

function OptimizedPathfinder:_on_success(message, path)
   log:debug('%s: %s', message, tostring(path))
   self._success_cb(path)
   self:stop()
end

function OptimizedPathfinder:_on_failure(message)
   log:debug(message)
   self:stop()
end

function OptimizedPathfinder:stop()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return OptimizedPathfinder
