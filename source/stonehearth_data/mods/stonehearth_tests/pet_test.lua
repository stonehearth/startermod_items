local MicroWorld = require 'lib.micro_world'
local rng = _radiant.csg.get_default_rng()

local PetTest = class(MicroWorld)

function PetTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local faction = 'civ'
   local humans = {}
   local critters = {}

   local worker1 = self:place_citizen(-7, -7)
   table.insert(humans, worker1)
   local worker2 = self:place_citizen( 7, -7)
   table.insert(humans, worker2)
   local worker3 = self:place_citizen(-7,  7)
   table.insert(humans, worker3)
   local trapper = self:place_citizen(7, 7, 'trapper')
   table.insert(humans, trapper)

   local critter1 = self:place_item('stonehearth:red_fox', 2, 2)
   table.insert(critters, critter1)
   local critter2 = self:place_item('stonehearth:racoon', -2, 2)
   table.insert(critters, critter2)
   local critter3 = self:place_item('stonehearth:rabbit', 2, -2)
   table.insert(critters, critter3)
   local critter4 = self:place_item('stonehearth:squirrel', -2, -2)
   table.insert(critters, critter4)

   self:place_item('stonehearth:berry_basket', 10, 10)
   self:place_item('stonehearth:berry_plate', -10, 10)
   self:place_item('stonehearth:berry_plate', 10, -10)
   self:place_item('stonehearth:berry_plate', -10, -10)
   self:place_item('stonehearth:arch_backed_chair', 5, 5)
   self:place_item('stonehearth:comfy_bed', -4, -4)

   local town = stonehearth.town:get_town(worker1)

   -- tame the critters
   for _, critter in pairs(critters) do
      local equipment = critter:add_component('stonehearth:equipment')
      equipment:equip_item('stonehearth:pet_collar')
      town:add_pet(critter)
   end

   self:at(1000,
      function()
         for _, critter in pairs(critters) do
            --critter:add_component('stonehearth:attributes'):set_attribute('calories', 25)
         end

         for _, human in pairs(humans) do
            --human:add_component('stonehearth:attributes'):set_attribute('calories', 25)
         end
      end
   )

   self:at(20000,
      function()
         for _, critter in pairs(critters) do
            --critter:add_component('stonehearth:attributes'):set_attribute('sleepiness', 120)
         end

         for _, human in pairs(humans) do
            --human:add_component('stonehearth:attributes'):set_attribute('sleepiness', 120)
         end
      end
   )
end

return PetTest
