local MicroWorld = require 'lib.micro_world'
local HarvestTest = class(MicroWorld)

-- VTUne license: CBL5-VT38CSF8

function HarvestTest:__init()
   self[MicroWorld]:__init(128)
   self:create_world()

   -- lotta logs.  let's get these into some stockpiles.
   self:place_item_cluster('stonehearth:oak_log', 32, 32, 20, 20)

   -- said stockpiles, all with different filters.  now we can't share filter
   -- functions!  NOOOOOOO.
   for x = -48,-8,8 do
      for z = -48,-8,8 do
         local stockpile = self:place_stockpile_cmd('player_1', x, z, 7, 7)
         local filter = {
            'wood',
         }
         table.insert(filter, string.format('unique value %d x %d', x, z))
         stockpile:get_component('stonehearth:stockpile')
                     :set_filter(filter)
      end
   end

   -- the worker gang.  these guys are going to ALL start bfs pathfinders and
   -- completely kill the simulation (until we fix it, that is!!)
   self:place_citizen(12, 12)
   self:place_citizen(12, 14)
   self:place_citizen(14, 12)
   self:place_citizen(14, 14)
   self:place_citizen(16, 12)
   self:place_citizen(16, 14)
   self:place_citizen(18, 12)
   self:place_citizen(18, 14)

end

return HarvestTest

