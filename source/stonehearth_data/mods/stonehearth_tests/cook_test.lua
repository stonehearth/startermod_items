local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local CookTest = class(MicroWorld)

function CookTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_citizen(-12, 4, 'farmer')
   local cook = self:place_citizen(-12, 7, 'cook')
   local player_id = radiant.entities.get_player_id(cook)

   -- put some items in the world
   self:place_item_cluster('stonehearth:resources:stone:hunk_of_stone', -10, 0, 2, 2, player_id)
   self:place_item('stonehearth:cook:talisman', 4, 1, player_id)
end

return CookTest
