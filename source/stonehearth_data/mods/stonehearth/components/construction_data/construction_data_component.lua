local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

local NINE_GRID_OPTION_TYPES = {
   nine_grid_gradiant = 'table',
   nine_grid_slope = 'number',
   nine_grid_max_height = 'number',
}

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

function ConstructionDataComponent:trace_data(reason, category)
   if category then
      return self.__saved_variables:trace_data(reason, category)
   end
   return self.__saved_variables:trace_data(reason)
end

function ConstructionDataComponent:_clone_writeable_options(into, from)
   into.normal = from.normal and Point3(from.normal) or nil
   into.nine_grid_region = from.nine_grid_region and Region2(from.nine_grid_region) or nil
   into.type = from.type
   into.material = from.material
   into.use_custom_renderer = from.use_custom_renderer
   into.needs_scaffolding = from.needs_scaffolding
   into.max_workers = from.max_workers
   into.allow_diagonal_adjacency = from.allow_diagonal_adjacency
   into.project_adjacent_to_base = from.project_adjacent_to_base
   into.allow_crouching_construction = from.allow_crouching_construction
   into.paint_through_blueprint = from.paint_through_blueprint
   into.nine_grid_slope = from.nine_grid_slope
   into.nine_grid_gradiant = from.nine_grid_gradiant
   into.nine_grid_max_height = from.nine_grid_max_height
   into.brush = from.brush
end


function ConstructionDataComponent:save_to_template()
   local result = {
      normal = self._sv.normal,
      nine_grid_region = self._sv.nine_grid_region2,
      needs_scaffolding = self._sv.needs_scaffolding,
      project_adjacent_to_base = self._sv.project_adjacent_to_base,
      nine_grid_region = self._sv.nine_grid_region,
      nine_grid_slope = self._sv.nine_grid_slope,
      nine_grid_gradiant = self._sv.nine_grid_gradiant,
      nine_grid_max_height = self._sv.nine_grid_max_height,
      loan_scaffolding_to = radiant.keys(self._sv._loaning_scaffolding_to),
   }
   return result
end

function ConstructionDataComponent:load_from_template(data, options, entity_map)
   if data.normal then
      self._sv.normal = Point3(data.normal.x, data.normal.y, data.normal.z)
   end
   if data.nine_grid_region then
      self._sv.nine_grid_region = Region2()
      self._sv.nine_grid_region:load(data.nine_grid_region)
   end
   self._sv.needs_scaffolding = data.needs_scaffolding
   self._sv.project_adjacent_to_base = data.project_adjacent_to_base
   if options.mode == 'preview' then
      self._sv.paint_through_blueprint = false
   end
   self:apply_nine_grid_options(data)

   for _, key in pairs(data.loan_scaffolding_to) do
      self:loan_scaffolding_to(entity_map[key])
   end

   self.__saved_variables:mark_changed()
end

function ConstructionDataComponent:clone_from(entity)
   if entity then
      local other_cd = entity:get_component('stonehearth:construction_data')
      self:_clone_writeable_options(self._sv, other_cd._sv)
      self.__saved_variables:mark_changed()
   end
end

-- changes properties in the construction data component.
-- 
--    @param options - the options to change.  
--
function ConstructionDataComponent:apply_nine_grid_options(options)
   if options then
      for name, val in pairs(options) do
         if NINE_GRID_OPTION_TYPES[name] == 'number' then
            self._sv[name] = tonumber(val)
         elseif NINE_GRID_OPTION_TYPES[name] == 'table' then
            self._sv[name] = val
         end
      end
      self.__saved_variables:mark_changed()
   end
end

function ConstructionDataComponent:begin_editing(entity)
   self:clone_from(entity)
   self._sv.fabricator_entity = nil
   self._sv.building_entity = nil
end

function ConstructionDataComponent:get_use_custom_renderer()
   return self._sv.use_custom_renderer
end

function ConstructionDataComponent:get_paint_through_blueprint()
   return self._sv.paint_through_blueprint
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

function ConstructionDataComponent:set_needs_scaffolding(enabled)
   self._sv.needs_scaffolding = enabled
   self.__saved_variables:mark_changed()
   return self
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

function ConstructionDataComponent:get_blueprint_entity()
   return self._sv.fabricator_entity:get_component('stonehearth:fabricator')
                                    :get_blueprint()
end

function ConstructionDataComponent:get_fabricator_entity()
   return self._sv.fabricator_entity
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

function ConstructionDataComponent:set_project_adjacent_to_base(enabled)
   self._sv.project_adjacent_to_base = enabled
   self.__saved_variables:mark_changed()
   return self
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

function ConstructionDataComponent:create_voxel_brush()
   if self._sv.brush then
      return voxel_brush_util.create_brush(self._sv)
   end
end

return ConstructionDataComponent
