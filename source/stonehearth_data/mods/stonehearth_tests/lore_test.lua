local MicroWorld = require 'lib.micro_world'
local HarvestTest = class(MicroWorld)

function HarvestTest:__init()
   self[MicroWorld]:__init(128)
   self:create_world()

   self:place_item('stonehearth:ancient_oak_tree', -45, -25)
   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item('stonehearth:ancient_juniper_tree', -45, 25)
   self:place_item('stonehearth:large_juniper_tree', -25, 25)
   self:place_item('stonehearth:medium_juniper_tree', -5, 25)
   self:place_item('stonehearth:small_juniper_tree',  15, 25)

   self:place_item('stonehearth:large_boulder_1',  -25, 5)
   self:place_item('stonehearth:medium_boulder_1', -15, 5)
   

   self:place_item('stonehearth:small_boulder',    5, 5)
       :add_component('mob'):turn_to(90)

   self:place_item('stonehearth:berry_bush', -25, 15)
   self:place_item('stonehearth:berry_bush', -15, 15)
   self:place_item('stonehearth:plants:silkweed',  -5, 15)

   local worker = self:place_citizen(12, 12)
   self:place_citizen(14, 14)

   local player_id = radiant.entities.get_player_id(worker)

   self:place_item('stonehearth:rabbit:statue', 0, 0)
   self:place_item('stonehearth:rabbit:lantern_relic', 15, 5, player_id, { force_iconic = false })

   self:place_item('stonehearth:decoration:wooden_garden_lantern', 19, 5, player_id, { force_iconic = false })

   
   self:at(10,  function()
         --self:place_stockpile_cmd(player_id, 12, 12, 4, 4)
      end)

   self:at(1000, function()
      stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 2, minute = 58 })
   end)
end

return HarvestTest

