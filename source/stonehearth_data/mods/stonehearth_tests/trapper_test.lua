local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local TrapperTest = class(MicroWorld)
--[[
   Instantiate a carpenter, a workbench, and a piece of wood.
   Turn out a wooden sword
]]

function TrapperTest:__init()
   self[MicroWorld]:__init()
   self:create_world()


   self:place_tree(-9, -9)
   local worker = self:place_citizen(-7, -7)
   local trapper = self:place_citizen(7, 7,'trapper')
   radiant.entities.set_faction(trapper, 'civ')
   local trapper = self:place_citizen(-7, 7,'trapper')
   radiant.entities.set_faction(trapper, 'civ')

   local rabbit = self:place_item('stonehearth:rabbit', 0, 0)
   local rabbit = self:place_item('stonehearth:rabbit', -3, -6)
   local rabbit = self:place_item('stonehearth:rabbit', 0, -6)

   self:place_item('stonehearth:berry_basket', 0, 0)
   self:place_item('stonehearth:berry_basket', 1, 1)
      

   local kit = self:place_item('stonehearth:trapper:knife', -5, -5)

   radiant.effects.run_effect(worker, '/stonehearth/data/effects/gib_effect')

   -- place the town standard for the trapper to "return home" to when dumping his inventory
   local town = stonehearth.town:get_town(worker)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   town:set_banner(banner_entity)
   radiant.terrain.place_entity(banner_entity, Point3(11, 0, 11))

   self:at(10,  function()
         self:place_stockpile_cmd('player_1', 12, 12, 4, 4)
         worker:get_component('stonehearth:attributes'):set_attribute('calories', stonehearth.constants.food.MALNOURISHED)
         rabbit:get_component('stonehearth:attributes'):set_attribute('calories', stonehearth.constants.food.MALNOURISHED)

      end)

end

return TrapperTest

