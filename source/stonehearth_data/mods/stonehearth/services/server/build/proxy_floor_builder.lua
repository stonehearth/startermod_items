local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local constants = require('constants').construction
local ProxyFloor = require 'services.server.build.proxy_floor'
local ProxyContainer = require 'services.server.build.proxy_container'
local ProxyBuilder = require 'services.server.build.proxy_builder'
local ProxyFloorBuilder = class()

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyFloorBuilder:__init()
   self._log = radiant.log.create_logger('builder')
end

function ProxyFloorBuilder:go(response)
   stonehearth.selection.select_xz_region()
      :set_cursor('stonehearth:cursors:create_floor')
      :use_manual_marquee(function(selector, box)
            return _radiant.client.create_voxel_node(1, Region3(box), 'materials/blueprint_gridlines.xml', Point3f(0.5, 0, 0.5))
         end)
      :done(function(selector, box)
            self:_add_floor_segment(box, response)
            selector:destroy()
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no region')            
         end)
      :go()

   return self
end

function ProxyFloorBuilder:create_building()
   local building = ProxyContainer(nil, 'stonehearth:entities:building')
   building:set_building(self._root_proxy)   
   return building
end

function ProxyFloorBuilder:create_floor(building)
   return ProxyFloor(building, 'stonehearth:entities:wooden_floor')
               :set_building(building)
end

function ProxyFloorBuilder:_add_floor_segment(box, response)   
   local merge_with = {}
   local contents = _physics:get_entities_in_cube(box)
   for id, entity in pairs(contents) do
      local cd = stonehearth.builder:get_construction_data_for(entity)
      if cd and cd.type == "floor" then
         local building = stonehearth.builder:get_building_for(entity)
         table.insert(merge_with, building)
      end
   end
   if #merge_with > 0 then
      local building = stonehearth.builder:merge_buildings(merge_with)
      local floor = self:create_floor(nil)
   else
      local building = self:create_building()
      local floor = self:create_floor(building)
      building:move_to(box.min)
      floor:get_region():modify(function(cursor)
            cursor:copy_region(Region3(Cube3(Point3(0, 0, 0),
                                             box.max - box.min)))
         end)

      stonehearth.builder:create_building(building)
      building:destroy()
   end
end

function ProxyFloorBuilder:destroy()
end

return ProxyFloorBuilder
