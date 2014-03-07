local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

function ConstructionDataComponent:__create(entity, json)
   self._entity = entity
   self._data = json
   if not self._data.material then
      self._data.material = 'wood resource'
   end
   self.__savestate = radiant.create_datastore(self._data)
   self.__savestate:modify_data(function (o)
          if o.normal then
             o.normal = Point3(o.normal.x, o.normal.y, o.normal.z)
          end
      end)
end

function ConstructionDataComponent:get_material()
   return self._data.material
end

function ConstructionDataComponent:needs_scaffolding()
   return self._data.needs_scaffolding
end

function ConstructionDataComponent:get_savestate()
   -- xxx: this isn't even a copy!  it's the *actual data*
   return self.__savestate:get_data()
end

function ConstructionDataComponent:set_normal(normal)
   assert(radiant.util.is_a(normal, Point3))
   self._data.normal = normal
   self.__savestate:mark_changed()
   return self
end

function ConstructionDataComponent:get_normal()
   return self._data.normal
end

function ConstructionDataComponent:set_finished(finished)
   local changed = self._data.finished ~= finished
   if changed then
      self._data.finished = finished
      self.__savestate:mark_changed()

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
   self.__savestate:mark_changed()
end

function ConstructionDataComponent:get_max_workers()
   if self._data.max_workers then
      return self._data.max_workers
   end
   return 0
end

function ConstructionDataComponent:set_fabricator_entity(fentity)
   self._data.fabricator_entity = fentity
   self.__savestate:mark_changed()

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
