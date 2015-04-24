local build_util = require 'lib.build_util'

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
   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.finished = false
      self._sv.active = false
      self._sv.teardown = false
   end
   self._log:debug('initialized construction_progress component')
end

function ConstructionProgress:destroy()
   self._log:debug('destroying construction_progress for %s', self._entity)
   self:unlink()   
end

function ConstructionProgress:clone_from(other)
   if other then
      local other_cp = other:get_component('stonehearth:construction_progress')

      self._sv.finished = other_cp.finished
      self._sv.active = other_cp.active
      self._sv.teardown = other_cp.teardown
   end
   return self
end

function ConstructionProgress:unlink()
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
      if finished and self._sv.teardown and self._sv._fabricator_component_name then
         radiant.entities.destroy_entity(self._entity)
      end
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
         self:get_fabricator_component()
                  :set_active(active)
      end
   end
end

function ConstructionProgress:get_active()
   return self._sv.active
end

function ConstructionProgress:set_teardown(teardown)
   assert(self._entity:is_valid())
   
   if teardown ~= self._sv.teardown then
      self._sv.teardown = teardown
      self.__saved_variables:mark_changed()

      radiant.events.trigger_async(self._entity, 'stonehearth:construction:teardown_changed', { 
         entity = self._entity
      })

      -- if we don't have a fabricator, some external agent is responsible for tweaking our
      -- finished bits (e.g. the stonehearth:building component)
      if self._sv.fabricator_entity then
         self:get_fabricator_component()
                     :set_teardown(teardown)
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
   return self
end

-- returns the fabricator entity which is using our blueprint as a reference
function ConstructionProgress:get_fabricator_entity()
   return self._sv.fabricator_entity
end

-- sets the fabricator entity which is using our blueprint as a reference
function ConstructionProgress:set_fabricator_entity(fabricator_entity, component_name)
   assert(component_name)
   self._sv._fabricator_component_name = component_name
   self._sv.fabricator_entity = fabricator_entity
   self.__saved_variables:mark_changed()
   return self
end

function ConstructionProgress:get_finished()
   return self._sv.finished
end

-- returns the fabricator component for this entity.  the exact name of the component
-- was specified during the :set_fabricator_entity() call.  for walls, columns, etc,
-- it's 'stonehearth:fabricator'.  for windows, lanterns, flags, etc. it's 
-- 'stonehearth:fixture_fabricator'.  both implement the same interface.
---
function ConstructionProgress:get_fabricator_component()
   if self._sv.fabricator_entity then
      assert(self._sv._fabricator_component_name)
      local component = self._sv.fabricator_entity:get_component(self._sv._fabricator_component_name)
      assert(component)
      return component
   end
end

function ConstructionProgress:save_to_template()
   return {
      building_entity            = build_util.pack_entity(self._sv.building_entity),
      fabricator_component_name  = self._sv._fabricator_component_name,
   }
end

function ConstructionProgress:load_from_template(template, options, entity_map)
   self._sv.building_entity            = build_util.unpack_entity(template.building_entity, entity_map)
   self._sv._fabricator_component_name = template.fabricator_component_name

   self.__saved_variables:mark_changed()
end

function ConstructionProgress:finish_restoring_template()
   -- this is a weird place to put the fabricator back on, but let's go for it.   
   if self._sv._fabricator_component_name == 'stonehearth:fabricator' then
      stonehearth.build:add_fabricator(self._entity)
   elseif self._sv._fabricator_component_name == 'stonehearth:fixture_fabricator' then
      self._entity:get_component('stonehearth:fixture_fabricator')
                     :finish_restoring_template()
   end
end

return ConstructionProgress
