local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local PlacementTest = class(MicroWorld)
--[[
   Instantiate a carpenter, a workbench, and a piece of wood.
   Turn out a wooden sword
]]

function PlacementTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local citizen = self:place_citizen(12, 12)
   self:place_item('stonehearth:comfy_bed', 1, 5)

   self:place_item('stonehearth:comfy_bed', 2, 5)
   self:place_item('stonehearth:wooden_garden_lantern', 4, 5)

   local citizen2 = self:place_citizen(-12, -12)
   local player_id = radiant.entities.get_player_id(citizen)
---[[
   self:place_item('stonehearth:comfy_bed', 0, 0)
   self:place_item('stonehearth:arch_backed_chair', 0, 0)
   self:place_item('stonehearth:comfy_bed', 1, 0)
   self:place_item('stonehearth:dining_table', 2, 0)
   self:place_item('stonehearth:picket_fence', 0, 1)
   self:place_item('stonehearth:picket_fence_gate', 1, 1)
   self:place_item('stonehearth:simple_wooden_chair', 2, 1)
   self:place_item('stonehearth:table_for_one', 3, 1)
   self:place_item('stonehearth:wooden_door', 4, 1)
   self:place_item('stonehearth:wooden_window_frame', 0, 2)
   self:place_item('stonehearth:picket_fence', 0, 3)
   self:place_item('stonehearth:picket_fence', 1, 3)
   self:place_item('stonehearth:picket_fence', 2, 3)
   self:place_item('stonehearth:picket_fence', 3, 3)
   self:place_item('stonehearth:picket_fence', 4, 3)
   self:place_item('stonehearth:picket_fence', 5, 3)
   self:place_item('stonehearth:picket_fence', 6, 3)

   self:place_item('stonehearth:firepit', 7, 3, player_id)
   self:place_item('stonehearth:firepit', 9, 3, player_id)

   local tree = self:place_tree(-5, -12)
--]]
   --TODO: note that this table disappears after it is removed
   --and placed again. Why?
   local table = radiant.entities.create_entity('stonehearth:dining_table')
   radiant.terrain.place_entity(table, Point3(10, 1, 10))

   if true then return false end
   
   self:at(5000, function()
      self:place_item('stonehearth:comfy_bed', 1, 5)
   end)

   self:at(10000, function()
      self:place_item('stonehearth:comfy_bed', 2, 5)
      radiant.terrain.remove_entity(table)
   end)

   self:at(15000, function()
      self:place_item('stonehearth:comfy_bed', 3, 5)
      radiant.terrain.place_entity(table, Point3(11, 1, 11))
   end)

   self:at(20000, function()
      self:place_item('stonehearth:comfy_bed', 4, 5)
   end)

   self:at(25000, function()
      --self:place_item('stonehearth:comfy_bed', 5, 5)
   end)

   self:at(30000, function()
      --self:place_item('stonehearth:comfy_bed', 6, 5)
   end)

   --self:place_stockpile_cmd(faction, 10, 10, 5, 5)
end

return PlacementTest

