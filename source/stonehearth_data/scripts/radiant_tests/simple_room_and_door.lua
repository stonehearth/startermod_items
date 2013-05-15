local gm = require 'radiant.core.gm'
local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'
local MicroWorld = require 'radiant_tests.micro_world'

local SimpleRoomAndDoor = class(MicroWorld)
gm:register_scenario('radiant.tests.simple_room_and_door', SimpleRoomAndDoor)

SimpleRoomAndDoor['radiant.md.create'] = function(self, bounds)
   self:create_world()
   self:place_citizen(12, 12)
   self:place_citizen(12, 13)
   self:place_citizen(12, 14)
   self:place_citizen(12, 15)
   self:place_item_cluster('module://stonehearth/resources/oak_tree/oak_log', 4, 8, 4, 4)
   self:place_stockpile_cmd(4, -12)
   
   local house = self:create_room_cmd(-10, -10, 7, 7)
   check:is_entity(house)
   local ec = om:get_component(house, 'entity_container')
   ec:iterate_children(function(id, child)
      if om:has_component(child, 'wall') then
         local wall = om:get_component(child, 'wall')
         local normal = wall:get_normal()
         if normal.x == 0 and normal.z == 1 then
            self:create_door_cmd(wall, 2, 0, 0)
         elseif normal.x == 1 and normal.z == 0 then
            self:create_door_cmd(wall, 0, 0, 2)
         end
         log:info('child wall %d  %s', id, tostring(child))
      end
   end)
   
   self:at(100, function() self:start_project_cmd(house) end)
end

