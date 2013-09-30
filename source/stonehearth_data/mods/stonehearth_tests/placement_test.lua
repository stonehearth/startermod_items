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
   local faction = citizen:get_component('unit_info'):get_faction()

   self:place_item('stonehearth.comfy_bed_proxy', 0, 0)
   self:place_item('stonehearth.arch_backed_chair_proxy', 0, 0)
   self:place_item('stonehearth.comfy_bed_proxy', 1, 0)
   self:place_item('stonehearth.dining_table_proxy', 2, 0)
   self:place_item('stonehearth.picket_fence_proxy', 0, 1)
   self:place_item('stonehearth.picket_fence_gate_proxy', 1, 1)
   self:place_item('stonehearth.simple_wooden_chair_proxy', 2, 1)
   self:place_item('stonehearth.table_for_one_proxy', 3, 1)
   self:place_item('stonehearth.wooden_door_proxy', 4, 1)
   self:place_item('stonehearth.wooden_window_frame_proxy', 0, 2)
   self:place_item('stonehearth.picket_fence_proxy', 0, 3)
   self:place_item('stonehearth.picket_fence_proxy', 1, 3)
   self:place_item('stonehearth.picket_fence_proxy', 2, 3)
   self:place_item('stonehearth.picket_fence_proxy', 3, 3)
   self:place_item('stonehearth.picket_fence_proxy', 4, 3)
   self:place_item('stonehearth.picket_fence_proxy', 5, 3)
   self:place_item('stonehearth.picket_fence_proxy', 6, 3)
   self:place_item('stonehearth.fire_pit_proxy', 7, 3)
   self:place_item('stonehearth.fire_pit', 9, 3)

   self:at(5000, function()
      self:place_item('stonehearth.comfy_bed_proxy', 1, 5)
   end)

   self:at(10000, function()
      self:place_item('stonehearth.comfy_bed_proxy', 2, 5)
   end)

   self:at(15000, function()
      self:place_item('stonehearth.comfy_bed_proxy', 3, 5)
   end)

   self:at(20000, function()
      self:place_item('stonehearth.comfy_bed_proxy', 4, 5)
   end)

   self:at(25000, function()
      self:place_item('stonehearth.comfy_bed_proxy', 5, 5)
   end)

   self:at(30000, function()
      self:place_item('stonehearth.comfy_bed_proxy', 6, 5)
   end)

   self:place_stockpile_cmd(faction, 10, 10, 5, 5)

end

return PlacementTest

