local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

function ConstructionDataComponent:initialize(entity, json)
   self._entity = entity
   self._data = json
   if not self._data.material then
      self._data.material = 'wood resource'
   end
   self.__saved_variables = radiant.create_datastore(self._data)
   self.__saved_variables:modify_data(function (o)
          if o.normal then
             o.normal = Point3(o.normal.x, o.normal.y, o.normal.z)
          end
      end)
end

function ConstructionDataComponent:restore(entity, saved_variables)
   self._entity = entity
   self.__saved_variables = saved_variables
end

function ConstructionDataComponent:get_material()
   return self._data.material
end

function ConstructionDataComponent:needs_scaffolding()
   return self._data.needs_scaffolding
end

function ConstructionDataComponent:get_savestate()
   -- xxx: this isn't even a copy!  it's the *actual data*
   return self.__saved_variables:get_data()
end

function ConstructionDataComponent:set_normal(normal)
   assert(radiant.util.is_a(normal, Point3))
   self._data.normal = normal
   self.__saved_variables:mark_changed()
   return self
end

function ConstructionDataComponent:get_normal()
   return self._data.normal
end

function ConstructionDataComponent:get_max_workers()
   if self._data.max_workers then
      return self._data.max_workers
   end
   return 0
end

function ConstructionDataComponent:set_fabricator_entity(fentity)
   self._data.fabricator_entity = fentity
   self.__saved_variables:mark_changed()

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
