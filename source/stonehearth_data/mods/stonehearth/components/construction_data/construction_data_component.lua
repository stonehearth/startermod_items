local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color3 = _radiant.csg.Color3

local log = radiant.log.create_logger('build')

function ConstructionDataComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv = json
      self._sv.initialized = true           
      self.__saved_variables:set_data(self._sv)

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

-- clone only the options which are mutable on the construction data component.  in a perfect
-- world, we would isolate the non-writaable from the writable ones so we could just do a
-- table copy, but such is life!
--
function ConstructionDataComponent:_clone_writeable_options(into, from)
end

function ConstructionDataComponent:clone_from(entity)
   if entity then
      local into = self._sv
      local from = entity:get_component('stonehearth:construction_data')._sv

      into.normal = from.normal and Point3(from.normal) or nil
      into.project_adjacent_to_base = from.project_adjacent_to_base

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

function ConstructionDataComponent:get_connected_to()
   return self._sv.connected_to
end

function ConstructionDataComponent:get_material()
   return self._sv.material
end

function ConstructionDataComponent:get_type()
   return self._sv.type
end

function ConstructionDataComponent:get_brush()
   return self._sv.brush
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

function ConstructionDataComponent:set_fabricator_entity(entity)
   assert(entity, 'no entity specified in :set_fabricator_entity()')
   self._sv.fabricator_entity = entity
   self.__saved_variables:mark_changed()
   return self
end

function ConstructionDataComponent:get_building_entity(entity)
   return self._sv.building_entity
end

function ConstructionDataComponent:set_building_entity(entity)
   assert(entity, 'no entity specified in :set_building_entity()')
   self._sv.building_entity = entity
   self.__saved_variables:mark_changed()
   return self
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

function ConstructionDataComponent:create_voxel_brush(brush, origin)
   checks('self', 'string', '?Point3')

   return voxel_brush_util.create_brush(brush, origin, self._sv.normal)
end

function ConstructionDataComponent:save_to_template()
   local result = {
      normal = self._sv.normal,
      project_adjacent_to_base = self._sv.project_adjacent_to_base,
   }
   return result
end

function ConstructionDataComponent:load_from_template(data, options, entity_map)
   if data.normal then
      self._sv.normal = Point3(data.normal.x, data.normal.y, data.normal.z)
   end
   self._sv.project_adjacent_to_base = data.project_adjacent_to_base

   self.__saved_variables:mark_changed()
end

function ConstructionDataComponent:rotate_structure(degrees)
   if self._sv.normal then
      self._sv.normal = self._sv.normal:rotated(degrees)
   end
   self.__saved_variables:mark_changed()
end


-- adds the `region` in world coordinates to the floor
--    @param brush_uri - the uri of the brush used to paint the floor
--    @param region - the region to add to the floor, in world coordinates
--
function ConstructionDataComponent:paint_on_world_region(brush_uri, world_region, replace)
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local brush = self:create_voxel_brush(brush_uri, origin)

   local shape = world_region:translated(-origin)
   local rgn = brush:paint_through_stencil(shape)

   return self:add_local_region(rgn, replace)
end

function ConstructionDataComponent:add_world_region(world_region)
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local shape = world_region:translated(-origin)

   self:add_local_region(shape)
   return self
end

function ConstructionDataComponent:add_local_region(rgn, replace)
   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(c)                           
                           if replace then
                              c:copy_region(rgn)
                           else
                              c:add_region(rgn)
                           end
                           c:optimize_by_merge('growing building structure')
                        end)         

   return self
end

function ConstructionDataComponent:remove_world_region(world_region)
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local shape = world_region:translated(-origin)

   self:remove_local_region(shape)
   return self
end

function ConstructionDataComponent:remove_local_region(shape)
   local region = self._entity:get_component('destination')
                                 :get_region()

   region:modify(function(c)                           
         c:subtract_region(shape)
         c:optimize_by_merge('shrinking building structure')
      end)         
   return self
end

return ConstructionDataComponent
