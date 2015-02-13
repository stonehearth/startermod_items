local constants = require('constants').construction
local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Region2 = _radiant.csg.Region2

local MODEL_OFFSET = Point3(-0.5, 0, -0.5)

local RoadEditor = class()


function RoadEditor:_region_to_road_regions(total_region, origin, build_curb)
   local proj_curb_region = Region2()
   local road_region = Region3()
   local curb_region = nil

   if build_curb and total_region:get_bounds():width() >= 3 and total_region:get_bounds():height() >= 3 then
      -- If we're big enough, add a curb along the edge.
      local edges = total_region:get_edge_list()
      curb_region = Region3()
      for edge in edges:each_edge() do
         local min = Point2(edge.min.location.x, edge.min.location.y)
         local max = Point2(edge.max.location.x, edge.max.location.y)

         if min.y == max.y then
            if edge.min.accumulated_normals.y < 0 then
               max.y = max.y + 1
            elseif edge.min.accumulated_normals.y > 0 then
               min.y = min.y - 1
            end
         elseif min.x == max.x then
            if edge.min.accumulated_normals.x < 0 then
               max.x = max.x + 1
            elseif edge.min.accumulated_normals.x > 0 then
               min.x = min.x - 1
            end
         end

         local c = Cube3(Point3(min.x, origin.y, min.y), Point3(max.x, 2 + origin.y, max.y))
         curb_region:add_cube(c)
      end

      proj_curb_region = curb_region:project_onto_xz_plane()
   end

   local proj_road_region = total_region - proj_curb_region

   for cube in proj_road_region:each_cube() do
      local c = Cube3(Point3(cube.min.x, origin.y, cube.min.y),
         Point3(cube.max.x, origin.y + 1, cube.max.y))
      road_region:add_cube(c)
   end

   return curb_region, road_region
end


-- this is the component which manages the fabricator entity.
function RoadEditor:__init(build_service)
   self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
   self._cut_region = _radiant.client.alloc_region3()
   _radiant.renderer.add_terrain_cut(self._cut_region)
end

function RoadEditor:go(response, road_uri, curb_uri)
   local road_brush_shape = voxel_brush_util.brush_from_uri(road_uri)
   local road_brush = _radiant.voxel.create_brush(road_brush_shape)

   local curb_brush_shape = nil
   local curb_brush = nil

   if curb_uri then
      curb_brush_shape = voxel_brush_util.brush_from_uri(curb_uri)
      curb_brush = _radiant.voxel.create_brush(curb_brush_shape)
   end
   stonehearth.selection:select_xz_region()
      :require_unblocked(false)
      :select_front_brick(false)
      :set_validation_offset(Point3.unit_y)
      :allow_select_cursor(true)
      :set_cursor('stonehearth:cursors:create_floor')
      :set_find_support_filter(stonehearth.selection.make_edit_floor_xz_region_support_filter(true))
      :set_can_contain_entity_filter(stonehearth.selection.floor_can_contain)
      :use_manual_marquee(function(selector, box)
            local proj_region = Region3(box):project_onto_xz_plane():to_int()
            local curb_region, road_region = self:_region_to_road_regions(proj_region, box.min, curb_uri ~= nil)
            local model_region = road_brush:paint_through_stencil(road_region)
            if curb_region and curb_uri then
               model_region:add_region(curb_brush:paint_through_stencil(curb_region))
            end
            local node =  _radiant.client.create_voxel_node(1, model_region, 'materials/blueprint.material.xml', Point3(0, 0, 0))
            node:set_polygon_offset(-5, -5)
            node:set_position(MODEL_OFFSET)

            self._cut_region:modify(function(cursor)
                  cursor:copy_region(road_region)
               end)
            return node
         end)
      :done(function(selector, box)
            self:_add_road(response, selector, box, road_uri, curb_uri)
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

function RoadEditor:_add_road(response, selector, box, road_uri, curb_uri)
   _radiant.call_obj(self._build_service, 'add_road_command', road_uri, curb_uri, box)
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