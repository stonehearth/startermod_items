local constants = require('constants').construction
local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Region2 = _radiant.csg.Region2

local MODEL_OFFSET = Point3(-0.5, 0, -0.5)

local RoadEditor = class()


-- this is the component which manages the fabricator entity.
function RoadEditor:__init(build_service)
   self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
   self._cut_region = _radiant.client.alloc_region3()
   _radiant.renderer.add_terrain_cut(self._cut_region)
end

function RoadEditor:go(response, road_uri)
   local road_brush = voxel_brush_util.create_brush(road_uri)

   stonehearth.selection:select_xz_region()
      :require_unblocked(false)
      :select_front_brick(false)
      :set_validation_offset(Point3.unit_y)
      :allow_select_cursor(true)
      :set_cursor('stonehearth:cursors:create_floor')
      :set_find_support_filter(stonehearth.selection.make_edit_floor_xz_region_support_filter(true))
      :set_can_contain_entity_filter(stonehearth.selection.floor_can_contain)
      :use_manual_marquee(function(selector, box)
            local road_region = Region3(box)
            local model_region = road_brush:paint_through_stencil(road_region)
            local node =  _radiant.client.create_voxel_node(H3DRootNode, model_region, 'materials/blueprint.material.json', Point3(0, 0, 0))
            node:set_polygon_offset(-5, -5)
            node:set_position(MODEL_OFFSET)

            self._cut_region:modify(function(cursor)
                  cursor:copy_region(road_region)
               end)
            return node
         end)
      :done(function(selector, box)
            self:_add_road(response, selector, box, road_uri)
         end)
      :fail(function(selector)
            response:reject('no region')            
         end)
      :always(function()
            if self._cut_region then
               _radiant.renderer.remove_terrain_cut(self._cut_region)
               self._cut_region = nil
            end
         end)
      :go()

   return self
end

function RoadEditor:_add_road(response, selector, box, road_uri)
   _radiant.call_obj(self._build_service, 'add_road_command', road_uri, box)
      :done(function(r)
            if r.new_selection then
               stonehearth.selection:select_entity(r.new_selection)
            end
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
      :always(function()
            if self._cut_region then
               _radiant.renderer.remove_terrain_cut(self._cut_region)
               self._cut_region = nil
            end
         end)
end

function RoadEditor:destroy()
end

return RoadEditor 