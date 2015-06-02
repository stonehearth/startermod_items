local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local ConstructionProgressClient = class()

function ConstructionProgressClient:initialize(entity)
   self._entity = entity
end

function ConstructionProgressClient:get_data()
   return self.__saved_variables:get_data()
end

function ConstructionProgressClient:get_teardown()
   return self:get_data().teardown
end

function ConstructionProgressClient:get_building_entity()
   return self:get_data().building_entity
end

function ConstructionProgressClient:get_fabricator_entity()
   return self:get_data().fabricator_entity
end

function ConstructionProgressClient:begin_editing(building, fabricator)
   -- xxx: assert we're in a RW store...
   self._color_region = radiant.alloc_region3()
   self.__saved_variables:modify(function (o)
         o.building_entity = building
         o.fabricator_entity = fabricator
      end)
end

function ConstructionProgressClient:load_from_template(template, options, entity_map)
end

-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!
-- XXX: NOOOOOOOOOOOOOOOOO CPOPYOY APSPA ATAT!!


-- adds the `region` in world coordinates to the floor
--    @param brush_uri - the uri of the brush used to paint the floor
--    @param region - the region to add to the floor, in world coordinates
--
function ConstructionProgressClient:create_voxel_brush(brush, origin)
   checks('self', 'string', '?Point3')

   return voxel_brush_util.create_brush(brush, origin, self._sv.normal)
end

function ConstructionProgressClient:paint_on_world_region(brush_uri, world_region, replace)
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local brush = self:create_voxel_brush(brush_uri, origin)

   local local_region = world_region:translated(-origin)
   local color_region = brush:paint_through_stencil(local_region)

   self:_update_destination_region(local_region, replace)
   self:_update_color_region(color_region, replace)

   return self
end


function ConstructionProgressClient:_update_destination_region(rgn, replace)
   assert(rgn:is_homogeneous())

   local dst_rgn = self._entity:get_component('destination')
                                 :get_region()

   assert(dst_rgn:get():is_homogeneous())
   dst_rgn:modify(function(cursor)
         if replace then
            cursor:copy_region(rgn)
         else
            cursor:add_region(rgn)
         end
         cursor:optimize_by_merge('growing building structure')
      end)
   assert(dst_rgn:get():is_homogeneous())
end

function ConstructionProgressClient:_update_color_region(rgn, replace)
   self._color_region:modify(function(cursor)
         if replace then
            cursor:copy_region(rgn)
         else
            cursor:add_region(rgn)
         end
         cursor:optimize_by_merge('growing building structure')
      end)
end


return ConstructionProgressClient
