local CombatService = class()
local Damage = require 'services.combat.damage'

function CombatService:__init()
   
end

function CombatService:resolve(damage)
   -- run an effect if it's a hit
   -- xxx, assumit it's a hit for now
   local attacker = damage:get_attacker()
   local target = damage:get_target()

   radiant.effects.run_effect(target, '/stonehearth/data/effects/hit_sparks/blood_effect.json')
   --radiant.effects.run_effect(target, 'on_hit')

   -- apply the damage
   self:apply_damage(attacker, target, damage.get_damage_amount())
end

function CombatService:add_damage(source_entity, target_entity) 
   return Damage(self, source_entity, target_entity)
end

function CombatService:apply_damage(source_entity, target_entity, damage)

   -- xxx, hack. slow down the damage taker. Move this to a debuff system
   radiant.entities.set_attribute(target_entity, 'speed', 30)

   -- apply the damage. xxx, hardcoded for now
   local health = radiant.entities.get_attribute(target_entity, 'health')
   health = health - damage

   if health <= 0 then
      --radiant.entities.kill_entity(target_entity)
   else 
      radiant.entities.set_attribute(target_entity, health)
   end
end

return CombatService()
