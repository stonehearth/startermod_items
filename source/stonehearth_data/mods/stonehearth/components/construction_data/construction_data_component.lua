local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

-- xxx: it would be nice if we could update the datastore once at an appropriate
-- time in the gameloop instead of everytime the normal or tangent changes.
function ConstructionDataComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
   self._data = {}
   self._data_binding:update(self._data)
end

function ConstructionDataComponent:get_data()
   return self._data
end

function ConstructionDataComponent:extend(data)
   self._data = data
   self._data_binding:update(self._data)
end

function ConstructionDataComponent:set_normal(normal)
   self._data.normal = normal
   self._data_binding:mark_changed()
   return self
end

function ConstructionDataComponent:set_finished(finished)
   local changed = self._data.finished ~= finished
   if changed then
      self._data.finished = finished
      self._data_binding:mark_changed()

      log:debug('%s trigger stonehearth:construction_finished event (finished = %s)',
                  self._entity, tostring(finished))

      radiant.events.trigger(self._entity, 'stonehearth:construction_finished', { 
         entity = self._entity,
         finished = finished   
      })
   end
   return self
end

function ConstructionDataComponent:get_finished()
   return self._data.finished
end

function ConstructionDataComponent:get_dependencies()
   return self._data.dependencies
end

function ConstructionDataComponent:add_dependency(dep)
   if not self._data.dependencies then
      self._data.dependencies = {}
   end
   self._data.dependencies[dep:get_id()] = dep
   self._data_binding:mark_changed()
end

function ConstructionDataComponent:get_max_workers()
   if self._data.max_workers then
      return self._data.max_workers
   end
   return 0
end

function ConstructionDataComponent:set_fabricator_entity(fentity)
   self._data.fabricator_entity = fentity
   self._data_binding:mark_changed()

   log:debug('%s trigger stonehearth:construction_fabricator_changed event (fabricator_entity = %s)',
               self._entity, fentity)

   radiant.events.trigger(self._entity, 'stonehearth:construction_fabricator_changed', { 
      entity = self._entity,
      fabricator_entity = fentity
   })
end

function ConstructionDataComponent:get_fabricator_entity()
   return self._data.fabricator_entity
end

function ConstructionDataComponent:get_allow_diagonal_adjacency()
   -- coearse to bool 
   return self._data.allow_diagonal_adjacency and true or false
end

function ConstructionDataComponent:get_project_adjacent_to_base()
   -- coearse to bool 
   return self._data.project_adjacent_to_base and true or false
end

return ConstructionDataComponent
