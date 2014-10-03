local MicroWorld = require 'lib.micro_world'
local QuestTest = class(MicroWorld)

function QuestTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item('stonehearth:large_juniper_tree', -25, -5)
   self:place_item('stonehearth:medium_juniper_tree', -5, -5)
   self:place_item('stonehearth:small_juniper_tree',  15, -5)

   self:place_item_cluster('stonehearth:oak_log', -10, 0, 10, 10)

   self:place_citizen(12, 12)
   self:place_citizen(14, 14)
   
   self:at(10,  function()
         --self:place_stockpile_cmd(player_id, 12, 12, 4, 4)
         stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:quests:collect_starting_resources')
      end)

   self:at(100, function()
         --tree:get_component('stonehearth:commands'):do_command('chop', player_id)
      end)
end

return QuestTest
