local MicroWorld = require 'lib.micro_world'
local MicroWorld = require 'lib.micro_world'
local BlacksmithTest = class(MicroWorld)

function BlacksmithTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   --self:place_item('stonehearth:footman:wooden_sword_talisman',  0, 24)
   --self:place_item_cluster('stonehearth:footman:wooden_sword_talisman', -4, 0, 2, 3)
   self:place_item('stonehearth:weapons:bronze_mace',  -2, 0)
   self:place_item('stonehearth:weapons:stone_maul',  -1, 0)
   self:place_item('stonehearth:weapons:iron_mace',  0, 0)
   --self:place_item('stonehearth:weapons:bronze_sword',  1, 0)
   --self:place_item('stonehearth:weapons:short_sword',  2, 0)
   --self:place_item('stonehearth:weapons:long_sword',  3, 0)

   local worker = self:place_citizen(12, 12)
   self:place_citizen(10, 14, 'footman')
   self:place_citizen(8, 14, 'footman')
   --self:place_citizen(14, 14, 'mason')

   local player_id = radiant.entities.get_player_id(worker)
   --self:place_item('stonehearth:decoration:stone_brazier', 1, 1, player_id, { force_iconic = false })
   --self:place_item('stonehearth:decoration:tower_brazier', 3, 1, player_id, { force_iconic = false })
   
   self:at(10,  function()
         self:place_stockpile_cmd(player_id, -16, -16, 32, 32)
      end)

end

return BlacksmithTest