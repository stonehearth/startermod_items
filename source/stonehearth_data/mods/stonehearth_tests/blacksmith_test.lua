local MicroWorld = require 'lib.micro_world'
local BlacksmithTest = class(MicroWorld)

function BlacksmithTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item('stonehearth:large_boulder_1',  -25, 5)
   self:place_item('stonehearth:medium_boulder_1', -15, 5)
   

   self:place_item('stonehearth:small_boulder',    5, 5)
       :add_component('mob'):turn_to(90)

   self:place_item('stonehearth:small_boulder',    15, 5)
       :add_component('mob'):turn_to(90)

   self:place_item_cluster('stonehearth:resources:wood:oak_log', 8, 8, 2, 2)
   self:place_item_cluster('stonehearth:resources:stone:hunk_of_stone', 8, 10, 2, 2)
   self:place_item_cluster('stonehearth:furniture:cobblestone_fence', 0, 0, 4, 8)
   self:place_item_cluster('stonehearth:furniture:cobblestone_fence_gate', -4, -4, 2, 2)

   local worker = self:place_citizen(12, 12)
   self:place_citizen(14, 14, 'blacksmith')
   --self:place_citizen(14, 14, 'carpenter')

   local player_id = radiant.entities.get_player_id(worker)
   self:place_item('stonehearth:decoration:stone_brazier', 1, 1, player_id, { force_iconic = false })
   self:place_item('stonehearth:decoration:tower_brazier', 3, 1, player_id, { force_iconic = false })
   
   self:at(2000,  function()
         --stonehearth.calendar:set_time_unit_test_only({ hour = 22, minute = 38 })
         --stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:candledark')
         --stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:quests:collect_starting_resources')   
      end)

end

return BlacksmithTest

