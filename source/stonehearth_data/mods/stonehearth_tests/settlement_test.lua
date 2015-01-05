local MicroWorld = require 'lib.micro_world'
local SettlementTest = class(MicroWorld)

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

function SettlementTest:__init()
   self[MicroWorld]:__init(1024)
   self:create_world()

   self:place_citizen(2, 2)


   local banner = radiant.entities.create_entity('stonehearth:camp_standard')
   radiant.terrain.place_entity(banner, Point3.zero, { force_iconic = false })
   stonehearth.town:get_town('player_1')
                     :set_banner(banner)
                     
   stonehearth.game_master:start()
   
   -- just for fun...
   --[[
   self:place_item_cluster('stonehearth:resources:wood:oak_log', 8, 8, 7, 7)
   radiant.set_realtime_timer(2000, function()
         local location = Point3(-4, 1, -4)
         local size = Point2(4, 4 )
         stonehearth.inventory:get_inventory('goblins')
                                 :create_stockpile(location, size)
      end)
   ]]

   if true then return end

   self:place_citizen(2, 2)
   self:place_citizen(2, 2)
   self:place_citizen(2, 2)
   self:place_citizen(2, 2)

   self:place_item_cluster('stonehearth:resources:wood:oak_log', 8, 8, 7, 7)
   self:place_item_cluster('stonehearth:resources:stone:hunk_of_stone', -8, 8, 7, 7)
   self:place_item_cluster('stonehearth:portals:wooden_door_2', -2, -2, 2, 2)
   self:place_item_cluster('stonehearth:decoration:wooden_wall_lantern', -10, 10, 2, 2)
   self:place_item_cluster('stonehearth:furniture:comfy_bed', 2, 2, 2, 2)
   if true then return end
   self:place_item_cluster('stonehearth:food:berries:berry_basket', -8, -8, 2, 2)
   self:place_item_cluster('stonehearth:portals:wooden_door', -8, 8, 1, 1)
   self:place_item_cluster('stonehearth:portals:wooden_window_frame', -12, 8, 2, 2)
   self:place_item_cluster('stonehearth:portals:wooden_diamond_window', -12, 12, 2, 2)
   self:place_item_cluster('stonehearth:decoration:green_hanging', 4, 2, 2, 2)
   
   --self:place_citizen(0, 0)
   if true then return end

   self:place_citizen(2, 2)
   self:place_citizen(2, 2)
   self:place_citizen(2, 2)
   self:place_citizen(2, 2)

   if true then return end   
   for i = -8, 8, 4 do
   self:place_citizen(0, i)
   end
end

return SettlementTest


