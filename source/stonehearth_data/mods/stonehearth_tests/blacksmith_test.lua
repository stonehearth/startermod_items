local MicroWorld = require 'lib.micro_world'
local BlacksmithTest = class(MicroWorld)

function BlacksmithTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item_cluster('stonehearth:resources:tin:ore', 4, 14, 2, 2)
   self:place_item_cluster('stonehearth:resources:silver:ore', 0, 14, 2, 2)
   self:place_item_cluster('stonehearth:resources:stone:hunk_of_stone', 8, 14, 2, 2)
   self:place_item_cluster('stonehearth:resources:copper:ore', -4, 8, 4, 4)
   self:place_item_cluster('stonehearth:resources:iron:ore', 0, 8, 4, 4)
   self:place_item_cluster('stonehearth:refined:steel_ingot', 4, 8, 4, 4)
   self:place_item_cluster('stonehearth:resources:coal:lump_of_coal', 8, 8, 4, 4)
   self:place_item_cluster('stonehearth:refined:thread', 0, 16, 2, 2)
   self:place_item_cluster('stonehearth:refined:leather_bolt', 0, 20, 2, 2)
   self:place_item('stonehearth:footman:wooden_sword_talisman',  0, 24)

   local worker = self:place_citizen(12, 12)
   self:place_citizen(14, 14, 'blacksmith')
   --self:place_citizen(14, 14, 'mason')

   local player_id = radiant.entities.get_player_id(worker)
   --self:place_item('stonehearth:decoration:stone_brazier', 1, 1, player_id, { force_iconic = false })
   --self:place_item('stonehearth:decoration:tower_brazier', 3, 1, player_id, { force_iconic = false })
   self:place_item('stonehearth:blacksmith:forge', 5, 1, player_id, { force_iconic = false })
   
   self:at(2000,  function()
         --stonehearth.calendar:set_time_unit_test_only({ hour = 23, minute = 38 })
         --stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:candledark')
         --stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:quests:collect_starting_resources')   
      end)

end

return BlacksmithTest

