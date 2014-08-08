local MicroWorld = require 'lib.micro_world'
local HarvestTest = class(MicroWorld)

function HarvestTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()


   local citizen = self:place_citizen(0, 0)
   --radiant.entities.add_buff(citizen, 'stonehearth:buffs:snared')
   radiant.entities.turn_to(citizen, 180)


   self:at(2000,  function()
      radiant.effects.run_effect(citizen, '/stonehearth/data/effects/level_up')
   end)

   self:at(5000,  function()
      radiant.effects.run_effect(citizen, '/stonehearth/data/effects/level_up')
   end)

   self:at(10000,  function()
      radiant.effects.run_effect(citizen, '/stonehearth/data/effects/level_up')
   end)

   self:at(15000,  function()
      radiant.effects.run_effect(citizen, '/stonehearth/data/effects/level_up')
   end)

end

return HarvestTest

