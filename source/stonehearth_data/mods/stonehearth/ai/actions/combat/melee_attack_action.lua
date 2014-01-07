local MeleeAttack = class()

MeleeAttack.name = 'melee attack!'
MeleeAttack.does = 'stonehearth:attack:melee'
MeleeAttack.version = 1
MeleeAttack.priority = 0

function MeleeAttack:__init(ai, entity, weapon)
   self._weapon = weapon
end

function MeleeAttack:run(ai, entity, target)
   assert(target)
   radiant.entities.turn_to_face(entity, target)

   local combat_service = radiant.mods.load('stonehearth').combat
   local effect = radiant.entities.get_entity_data(self._weapon, "stonehearth:melee_weapon").effect
   
   local combat = combat_service
      :add_damage(entity, target)
      :add_damage_source(self._weapon)

   if effect then
      ai:execute('stonehearth:run_effect', effect, function ()
            combat_service:resolve(combat)
         end)
   else 
      ai:abort('no effect supplied for weapon %s', tostring(self._weapon))
   end
end

return MeleeAttack