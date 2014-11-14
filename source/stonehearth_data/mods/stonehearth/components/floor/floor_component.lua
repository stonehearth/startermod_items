local build_util = require 'lib.build_util'
local constants = require('constants').construction

local Floor = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

-- called to initialize the component on creation and loading.
--
function Floor:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity
   self._sv.category = constants.floor_category.FLOOR
end

function Floor:get_region()
   return self._entity:get_component('destination'):get_region()
end

-- adds the `region` in world coordinates to the floor
--    @param region - the region to add to the floor, in world coordinates
--
function Floor:add_region_to_floor(region, brush_shape)
   local building = self._entity:get_component('mob'):get_parent()
   local origin = radiant.entities.get_world_grid_location(building)
   local brush = _radiant.voxel.create_brush(brush_shape)
                                    :set_origin(origin)

   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(c)                           
                           local shape = region:translated(-origin)
                           local floor = brush:paint_through_stencil(shape)
                           c:add_region(floor)
                           c:optimize_by_merge()
                        end)         

   return self
end

function Floor:layout()
   -- nothing to do...
end

function Floor:set_category(category)
   self._sv.category = category
end

function Floor:get_category()
   return self._sv.category
end

function Floor:remove_region_from_floor(region)
   local building = self._entity:get_component('mob'):get_parent()
   local origin = radiant.entities.get_world_grid_location(building)

   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(c)                           
                           local shape = region:translated(-origin)
                           c:subtract_region(shape)
                           c:optimize_by_merge()
                        end)         

   return self
end

function Floor:clone_from(entity)
   if entity then
      local other_floor_region = entity:get_component('destination'):get_region():get()

      self._entity:get_component('destination')
                     :get_region()
                        :modify(function(r)
                           r:copy_region(other_floor_region)
                        end)
      end
   return self
end

function Floor:save_to_template()
   return {}
end

function Floor:load_from_template(data, options, entity_map)
   -- nothing to do!   
end

function Floor:rotate_structure(degrees)
   build_util.rotate_structure(self._entity, degrees)
end

return Floor
