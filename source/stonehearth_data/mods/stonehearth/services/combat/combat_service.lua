local CombatService = class()
local Damage = require 'services.combat.damage'

radiant.events.register_event('radiant.events.combat.damage')

function CombatService:__init()

end

function CombatService:do_damage(source_entity, target_entity) 
   return Damage(self, source_entity, target_entity)
end

function CombatService:apply_damage(source_entity, target_entity, damage)
   radiant.events.broadcast_msg('radiant.events.combat.damage', source_entity, target_entity, damage)
end

return CombatService()
