local MicroWorld = require 'lib.micro_world'
local EntTest = class(MicroWorld)

function EntTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   local ent = self:place_item('stonehearth:monsters:forest:ent', 0, 0)
   radiant.entities.turn_to(ent, 180)
   self:place_citizen(14, 14)
   
   self:at(10,  function()
         --self:place_stockpile_cmd(player_id, 12, 12, 4, 4)
      end)

   self:at(100, function()
         --tree:get_component('stonehearth:commands'):do_command('chop', player_id)
      end)
end

return EntTest

