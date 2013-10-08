local CombatService = class()
local Damage = require 'services.combat.damage'

radiant.events.register_event('radiant:events:combat.damage')

function CombatService:__init()

end

function CombatService:do_damage(source_entity, target_entity) 
   return Damage(self, source_entity, target_entity)
end

function CombatService:apply_damage(source_entity, target_entity, damage)
   radiant.events.broadcast_msg('radiant:events:combat.damage', source_entity, target_entity, damage)

   -- xxx, hack. slow down the damage taker. Move this to a debuff system
   radiant.entities.set_attribute(target_entity, 'speed', 30)

   -- pyrotechnics
   local spark_effect = radiant.effects.run_effect(target_entity, '/stonehearth/data/effects/hit_sparks/blood_effect.json')
   

   -- apply the damage. xxx, hardcoded for now
   local health = radiant.entities.get_attribute(target_entity, 'health')
   health = health - damage:get_damage_amount()

   if health <= 0 then
      radiant.combat.kill_entity(target_entity)
   else 
      radiant.entities.set_attribute(target_entity, health)
   end
end

return CombatService()
