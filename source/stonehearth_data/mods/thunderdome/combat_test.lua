local MicroWorld = require 'lib.micro_world'
local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local p1 = self:place_citizen(-2, 0)
   radiant.entities.turn_to(p1, 270)

   local p2 = self:place_citizen( 2, 0)
   radiant.entities.turn_to(p2, 90)

   radiant.events.trigger(p1, 'thunderdome:set_partner', { 
      entity = p2
   })

   radiant.events.trigger(p2, 'thunderdome:set_partner', { 
      entity = p1
   })
   
   _radiant.call('thunderdome:set_players', p1:get_id(), p2:get_id())

   --radiant.entities.destroy_entity(p2)

end

return CombatTest

