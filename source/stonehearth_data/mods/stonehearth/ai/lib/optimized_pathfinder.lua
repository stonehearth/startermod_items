local OptimizedPathfinder = class()

function OptimizedPathfinder:__init(log, entity, destination, success_cb, failure_cb)
   self._log = log
   self._entity = entity
   self._destination = destination
   self._success_cb = success_cb
   self._failure_cb = failure_cb
   self._start_location = nil
   self._use_direct_pathfinder = radiant.util.get_config('enable_direct_pathfinder', true)
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

   if self._use_direct_pathfinder then
      if self._log:is_enabled(radiant.log.DEBUG) then
         local destination_location = radiant.entities.get_world_grid_location(self._destination)
         self._log:debug('finding path from CURRENT.location %s to %s (@ %s)',
                          self._start_location, self._destination, destination_location)
      end

      local direct_path_finder = _radiant.sim.create_direct_path_finder(self._entity)
                                    :set_start_location(self._start_location)
                                    :set_destination_entity(self._destination)

      local path = direct_path_finder:get_path()

      if path then
         self:_on_success('direct path found', path)
         return
      end
   end

   self._log:detail('no direct path found.  starting a* pathfinder.')
   
   self._trace = radiant.entities.trace_grid_location(self._destination, 'OptimizedPathfinder')
      :on_changed(function()
         self:_on_failure('destination changed')
      end)
      :on_destroyed(function()
         self:_on_failure('destination destroyed')
      end)

   local solved = function(path)
      self:_on_success('a* path found', path)
      return true
   end

   self._description = string.format('find %s', tostring(self._destination))
   self._pathfinder = _radiant.sim.create_astar_path_finder(self._entity, self._description)
                         :set_source(self._start_location)
                         :add_destination(self._destination)
                         :set_solved_cb(solved)
                         :start()

   self._log:error('creating astar pathfinder for %s @ %s', self._description, self._start_location)

   return self
end

function OptimizedPathfinder:_on_success(message, path)
   self._log:debug('%s: %s', message, tostring(path))
   self._success_cb(path)
   self:stop()
end

function OptimizedPathfinder:_on_failure(message)
   self._log:debug(message)
   self:stop()
end

function OptimizedPathfinder:stop()
   if self._pathfinder then
      self._log:error('destroying astar pathfinder for %s @ %s', self._description, self._start_location)
      self._pathfinder:destroy()
      self._pathfinder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return OptimizedPathfinder
