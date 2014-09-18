local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local PromoteTest = class(MicroWorld)
--[[
   Instantiate a worker and a workbench with saw. Promote the worker into a carpenter
]]

function PromoteTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Create the saw 
   local saw = self:place_item('stonehearth:carpenter:saw_talisman', 10, 10)
   local saw = self:place_item('stonehearth:footman:wooden_sword_talisman', 12, 1)

   self:place_citizen(-1,2)
   local worker = self:place_citizen(12, 12)
 
   self:place_item_cluster('stonehearth:oak_log', 6, 6, 2, 2);
   
   local tree = self:place_tree(-8, 0)
   local tree2 = self:place_tree(-8, 8)

   local player_id = radiant.entities.get_player_id(worker)

   --Test for destroying promotion items, does not currently work
   --TODO: fix before wildfire mod destroys all kinds of random stuff. 
   --self:at(10000,  function()
   --     radiant.entities.destroy_entity(saw)
         --self:place_stockpile_cmd(player_id, 12, 12, 4, 4)
   --   end)
end

return PromoteTest
