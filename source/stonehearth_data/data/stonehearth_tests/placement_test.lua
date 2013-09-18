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

   self:place_item('stonehearth_items', 'comfy_bed_proxy', 0, 0)

   self:at(5000, function()
      self:place_item('stonehearth_items', 'comfy_bed_proxy', 1, 0)
   end)

   self:at(10000, function()
      self:place_item('stonehearth_items', 'comfy_bed_proxy', 2, 0)
   end)

   self:at(15000, function()
      self:place_item('stonehearth_items', 'comfy_bed_proxy', 3, 0)
   end)

   self:at(20000, function()
      self:place_item('stonehearth_items', 'comfy_bed_proxy', 4, 0)
   end)

   self:at(25000, function()
      self:place_item('stonehearth_items', 'comfy_bed_proxy', 5, 0)
   end)

   self:at(30000, function()
      self:place_item('stonehearth_items', 'comfy_bed_proxy', 6, 0)
   end)

   self:place_stockpile_cmd(faction, 10, 10, 2, 2)

end

return PlacementTest

