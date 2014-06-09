local ConstructionProgress = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3


function ConstructionProgress:initialize(entity, json)
   self._log = radiant.log.create_logger('build')
                     :set_prefix('con progress')
                     :set_entity(entity)

   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.dependencies then
      self._sv.dependencies = {}
      self._sv.inverse_dependences = {}
      self._sv.finished = false
      self._sv.active = false
      self._sv.teardown = false
      self._sv.dependencies_finished = false
   end
   
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         for id, blueprint in pairs(self._sv.dependencies) do
            self:_listen_for_changes(blueprint)
         end
         for id, blueprint in pairs(self._sv.inverse_dependences) do            
            self:_listen_for_changes(blueprint)
         end
         self:check_dependencies()
      end)
end

function ConstructionProgress:_listen_for_changes(blueprint)
   radiant.events.listen(blueprint, 'stonehearth:construction:teardown_changed', function()
         self:check_dependencies()
      end)
   radiant.events.listen(blueprint, 'stonehearth:construction:finished_changed', function()
         self:check_dependencies()
      end)
end

--- Tracks projects which must be completed before this one can start.
function ConstructionProgress:add_dependency(blueprint)
   assert(self._entity ~= blueprint)

   -- Create a new dependency tracking the blueprint. 
   local id = blueprint:get_id()

   self._sv.dependencies[id] = blueprint
   self.__saved_variables:mark_changed()

   blueprint:add_component('stonehearth:construction_progress')
            :_add_inverse_dependency(self._entity)

   self:_listen_for_changes(blueprint)
   self:check_dependencies()
   return self
end

function ConstructionProgress:_add_inverse_dependency(blueprint)
   assert(self._entity ~= blueprint)
   
   local id = blueprint:get_id()
   self._sv.inverse_dependences[id] = blueprint
   self.__saved_variables:mark_changed()

   self:_listen_for_changes(blueprint)
   self:check_dependencies()
   return self
end

function ConstructionProgress:check_dependencies()
   local last_dependencies_finished = self._sv.dependencies_finished  
   local teardown = self:get_teardown()

   local dependencies
   self._sv.dependencies_finished = true

   -- if we're building up the project, we should only proceed with contruction when
   -- all the dependencies which are also being built up have finished building.
   -- if we're tearing down, we need to wait for everyone who depends on us to be torn
   -- town first. 
   if teardown then
      self._log:debug('checking inverse dependencies...(current: %s  teardown: %s)', tostring(last_dependencies_finished), tostring(teardown))
      dependencies = self._sv.inverse_dependences
   else 
      self._log:debug('checking dependencies...(current: %s  teardown: %s)', tostring(last_dependencies_finished), tostring(teardown))
      dependencies = self._sv.dependencies
   end

   for id, blueprint in pairs(dependencies) do
      local progress = blueprint:get_component('stonehearth:construction_progress')
      local dep_teardown = progress:get_teardown()
      if dep_teardown  == teardown then
         if not progress:get_finished() then
            -- The project is actually not finished.  Definitely bail!
            self._log:debug('  blueprint %s not finished (teardown: %s).', blueprint, tostring(teardown))
            self._sv.dependencies_finished = false
            break
         else
            self._log:debug('  blueprint %s is finished (teardown: %s).', blueprint, tostring(teardown))
         end
      else
         self._log:debug('  blueprint %s doesnt match our teardown (teardown: %s, dep_teardown: %s).', blueprint, tostring(teardown), tostring(dep_teardown))
      end
   end
   
   -- Start or stop the project if the _dependencies_finished bit has flipped.
   if last_dependencies_finished ~= self._sv.dependencies_finished then
      self._log:debug('trigger stonehearth:construction:dependencies_finished_changed event (finished = %s)',
                       tostring(self._sv.dependencies_finished))
                  
      radiant.events.trigger_async(self._entity, 'stonehearth:construction:dependencies_finished_changed', { 
         entity = self._entity
      })

      -- if we don't have a fabricator, forward the message along.
      if not self._sv._fabricator_component_name then
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

      self._log:debug('trigger stonehearth:construction:finished_changed event (finished = %s)',
                      tostring(finished))
      
      radiant.events.trigger_async(self._entity, 'stonehearth:construction:finished_changed', { 
         entity = self._entity
      })
   end
   return self
end

-- set the active states.  workers will only try to build building that are active.
-- simply forwards the request to the fabricator of the fabricator entity.  we duplicate
-- the active state here as a convienence to clients
function ConstructionProgress:set_active(active)
   if active ~= self._sv.active then
      if self._sv.fabricator_entity then
         self._sv.active = active
         self.__saved_variables:mark_changed()
         self:_get_fabricator_component():set_active(active)
      end
   end
end

function ConstructionProgress:get_active()
   return self._sv.active
end

function ConstructionProgress:set_teardown(teardown)
   if teardown ~= self._sv.teardown then
      if self._sv.fabricator_entity then
         self._sv.teardown = teardown
         self.__saved_variables:mark_changed()
         self:_get_fabricator_component():set_active(active):set_teardown(teardown)

         radiant.events.trigger_async(self._entity, 'stonehearth:construction:teardown_changed', { 
            entity = self._entity
         })
         self:check_dependencies()
      end
   end
end

function ConstructionProgress:get_teardown()
   return self._sv.teardown
end

-- return the entity representing the buliding for which this wall, column,
-- floor, etc. is a part of.  is usefull for getting *way* deep in the structual
-- tree up to the construction blueprint which is actually interesting to the
-- user.
function ConstructionProgress:get_building_entity()
   return self._sv.building_entity
end

-- sets the entity representing the buliding for which this wall, column,
-- floor, etc. is a part of.  is usefull for getting *way* deep in the structual
-- tree up to the construction blueprint which is actually interesting to the
-- user.
function ConstructionProgress:set_building_entity(building_entity)
   self._sv.building_entity = building_entity
   self.__saved_variables:mark_changed()

   if self._sv.fabricator_entity then
      local project = self:_get_fabricator_component():get_project()
      if project then
         project:add_component('stonehearth:construction_data')
                :set_building_entity(building_entity)
      end
   end
end

-- returns the fabricator entity which is using our blueprint as a reference
function ConstructionProgress:get_fabricator_entity()
   return self._sv.fabricator_entity
end

-- sets the fabricator entity which is using our blueprint as a reference
function ConstructionProgress:set_fabricator_entity(fabricator_entity, component_name)
   self._sv._fabricator_component_name = component_name or 'stonehearth:fabricator'
   self._sv.fabricator_entity = fabricator_entity
   self.__saved_variables:mark_changed()

   if self._sv.building_entity then
      local project = self:_get_fabricator_component():get_project()
      if project then
         project:add_component('stonehearth:construction_data')
                :set_building_entity(self._sv.building_entity)
      end
   end
   return self
end

function ConstructionProgress:get_finished()
   return self._sv.finished
end

function ConstructionProgress:get_dependencies_finished()
   return self._sv.dependencies_finished
end

-- returns the fabricator component for this entity.  the exact name of the component
-- was specified during the :set_fabricator_entity() call.  for walls, columns, etc,
-- it's 'stonehearth:fabricator'.  for windows, lanterns, flags, etc. it's 
-- 'stonehearth:fixture_fabricator'.  both implement the same interface.
---
function ConstructionProgress:_get_fabricator_component()
   assert(self._sv.fabricator_entity)
   assert(self._sv._fabricator_component_name)
   local component = self._sv.fabricator_entity:get_component(self._sv._fabricator_component_name)
   assert(component)
   return component
end

return ConstructionProgress
