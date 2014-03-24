local Damage = class()

function Damage:__init(combat_service, attacking_entity, target_entity)
   self._combat_service = combat_service
   self._attacking_entity = attacking_entity
   self._target_entity = target_entity
   self._damage_sources = {}
end

function Damage:add_damage_source(damage_source)
   table.insert(self._damage_sources, damage_source)
   --self._hit_effect = radiant.entities.get_entity_data(damage_source, "stonehearth:melee_weapon").effect
   return self;   
end

function Damage:get_attacker()
   return self._attacking_entity
end

function Damage:get_target()
   return self._target_entity
end

function Damage:get_damage_amount()
   return 10 --xxx hardcoded
end

function Damage:get_hit_effect()
   return self._hit_effect
end

return Damage
