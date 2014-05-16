local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

function ConstructionDataComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv = json
      self._sv.initialized = true
      self._sv._loaning_scaffolding_to = {}
      self.__saved_variables:set_data(self._sv)

      if not self._sv.material then
         self._sv.material = 'wood resource'
      end
      if self._sv.normal then
         self._sv.normal = Point3(self._sv.normal.x, self._sv.normal.y, self._sv.normal.z)
      end
   end
end

function ConstructionDataComponent:get_use_custom_renderer()
   return self._sv.use_custom_renderer
end

function ConstructionDataComponent:get_connected_to()
   return self._sv.connected_to
end

function ConstructionDataComponent:get_material()
   return self._sv.material
end

function ConstructionDataComponent:get_type()
   return self._sv.type
end

function ConstructionDataComponent:needs_scaffolding()
   return self._sv.needs_scaffolding
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

function ConstructionDataComponent:set_nine_grid_region2(region2)
   self._sv.nine_grid_region = region2
   self.__saved_variables:mark_changed()
   return self
end

function ConstructionDataComponent:get_normal()
   return self._sv.normal
end

function ConstructionDataComponent:get_max_workers()
   if self._sv.max_workers then
      return self._sv.max_workers
   end
   return 4 -- completely arbitrary!  add a config option?
end

function ConstructionDataComponent:set_fabricator_entity(fentity)
   self._sv.fabricator_entity = fentity
   self.__saved_variables:mark_changed()
end

function ConstructionDataComponent:set_building_entity(entity)
   self._sv.building_entity = entity
   self.__saved_variables:mark_changed()
end

function ConstructionDataComponent:get_building_entity(entity)
   return self._sv.building_entity
end

function ConstructionDataComponent:get_allow_diagonal_adjacency()
   -- coearse to bool 
   return self._sv.allow_diagonal_adjacency and true or false
end

function ConstructionDataComponent:get_project_adjacent_to_base()
   -- coearse to bool 
   return self._sv.project_adjacent_to_base and true or false
end

function ConstructionDataComponent:get_allow_crouching_construction()
   return self._sv.allow_crouching_construction and true or false
end

-- used to loan our scaffolding to the borrower.  this means we won't
-- try to tear down the scaffolding until our entity and the borrower
-- entity are both finished.
function ConstructionDataComponent:loan_scaffolding_to(borrower)
   if borrower and borrower:is_valid() then
      self._sv._loaning_scaffolding_to[borrower:get_id()] = borrower
      self.__saved_variables:mark_changed()
   end
end

-- return the map of entities that we're loaning our scaffolding to
function ConstructionDataComponent:get_loaning_scaffolding_to()
   return self._sv._loaning_scaffolding_to
end

function ConstructionDataComponent:create_voxel_brush(paint_mode)
   if self._sv.brush then
      if not paint_mode then
         paint_mode = self._sv.paint_mode
      end
      return voxel_brush_util.create_brush(self._sv, paint_mode)
   end
end

return ConstructionDataComponent
