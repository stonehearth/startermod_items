local MicroWorld = require 'lib.micro_world'
local MasonTest = class(MicroWorld)

function MasonTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   
   self:place_citizen(12, 12)
   self:place_citizen(14, 14, 'mason')

   self:place_item_cluster('stonehearth:resources:wood:oak_log', -10, 0, 10, 10)
   self:place_item_cluster('stonehearth:hunk_of_stone', 10, 3, 3, 3)   
   
   self:at(10,  function()
         --self:place_stockpile_cmd(player_id, 12, 12, 4, 4)
      end)

   self:at(100, function()
         --tree:get_component('stonehearth:commands'):do_command('chop', player_id)
      end)
end

return MasonTest

