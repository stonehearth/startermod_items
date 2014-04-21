local ConstructionProgress = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

function ConstructionProgress:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.dependencies then
      self._sv.dependencies = {}
      self._sv.finished = false
      self._sv.dependencies_finished = false
   end
   
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         for id, blueprint in pairs(self._sv.dependencies) do
            radiant.events.listen(blueprint, 'stonehearth:construction:finished_changed', function()
                  self:check_dependencies()
               end)
         end
         self:check_dependencies()
      end)   
end

--- Tracks projects which must be completed before this one can start.
function ConstructionProgress:add_dependency(blueprint)
   -- Create a new dependency tracking the blueprint. 
   local id = blueprint:get_id()

   self._sv.dependencies[id] = blueprint
   self.__saved_variables:mark_changed()

   radiant.events.listen(blueprint, 'stonehearth:construction:finished_changed', function()
         self:check_dependencies()
      end)
   self:check_dependencies()
end

function ConstructionProgress:check_dependencies()
   local last_dependencies_finished = self._sv.dependencies_finished
   
   log:debug('%s checking dependencies...', self._entity)

   -- Assume we're good to go.  If not, the loop below will catch it.
   self._sv.dependencies_finished = true
   for id, blueprint in pairs(self._sv.dependencies) do
      local progress = blueprint:get_component('stonehearth:construction_progress')
      if progress and not progress:get_finished() then
         -- The project is actually not finished.  Definitely bail!
         log:debug('%s blueprint %s not finished.', self._entity, blueprint)
         self._sv.dependencies_finished = false
         break
      end
   end
   
   -- Start or stop the project if the _dependencies_finished bit has flipped.
   if last_dependencies_finished ~= self._sv.dependencies_finished then
      log:debug('%s trigger stonehearth:construction:dependencies_finished_changed event (finished = %s)',
                  self._entity, tostring(self._sv.dependencies_finished))
                  
      radiant.events.trigger_async(self._entity, 'stonehearth:construction:dependencies_finished_changed', { 
         entity = self._entity
      })

      -- if we don't have a construction_data component, forward the message along.
      local construction_data = self._entity:get_component('stonehearth:construction_data')
      if not construction_data then
         self:set_finished(self._sv.dependencies_finished)
      end
   end
   return self._sv.dependencies_finished
end

-- used by the fabricator to indicate the construction is done
function ConstructionProgress:set_finished(finished)
   local changed = self._sv.finished ~= finished
   if changed then
      self._sv.finished = finished
      self.__saved_variables:mark_changed()

      log:debug('%s trigger stonehearth:construction:finished_changed event (finished = %s)',
                  self._entity, tostring(finished))
      
      radiant.events.trigger_async(self._entity, 'stonehearth:construction:finished_changed', { 
         entity = self._entity
      })
   end
   return self
end

function ConstructionProgress:get_finished()
   return self._sv.finished
end

function ConstructionProgress:get_dependencies_finished()
   return self._sv.dependencies_finished
end

return ConstructionProgress
