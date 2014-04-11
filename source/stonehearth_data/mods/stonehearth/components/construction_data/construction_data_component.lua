local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

function ConstructionDataComponent:initialize(entity, json)
   self._entity = entity
   self._data = json
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv = json
      self._sv._initialized = true
      self.__saved_variables:set_data(self._sv)
      if not self._sv.material then
         self._sv.material = 'wood resource'
      end
      if self._sv.normal then
         self._sv.normal = Point3(self._sv.normal.x, self._sv.normal.y, self._sv.normal.z)
      end
   end
end

function ConstructionDataComponent:get_material()
   return self._data.material
end

function ConstructionDataComponent:needs_scaffolding()
   return self._data.needs_scaffolding
end

function ConstructionDataComponent:get_savestate()
   -- xxx: this isn't even a copy!  it's the *actual data*
   return self._sv
end

function ConstructionDataComponent:set_normal(normal)
   assert(radiant.util.is_a(normal, Point3))
   self._sv.normal = normal
   self.__saved_variables:mark_changed()
   return self
end

function ConstructionDataComponent:get_normal()
   return self._sv.normal
end

function ConstructionDataComponent:get_max_workers()
   if self._data.max_workers then
      return self._data.max_workers
   end
   return 0
end

function ConstructionDataComponent:set_fabricator_entity(fentity)
   self._sv.fabricator_entity = fentity
   self.__saved_variables:mark_changed()
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
