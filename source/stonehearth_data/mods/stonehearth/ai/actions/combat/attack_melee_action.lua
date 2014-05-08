local EngageContext = require 'services.server.combat.engage_context'
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local AttackMelee = class()

AttackMelee.name = 'melee attack'
AttackMelee.does = 'stonehearth:combat:attack'
AttackMelee.args = {
   target = Entity
}
AttackMelee.version = 2
AttackMelee.priority = 1
AttackMelee.weight = 1

-- TODO: rewrite this as a compound action so we don't execute the astar path finder in run
function AttackMelee:run(ai, entity, args)
   local target = args.target

   if not target:is_valid() then
      ai:abort('target has been destroyed')
      return
   end

   local weapon = radiant.entities.get_equipped_item(entity, 'mainHand')
   local weapon_data = radiant.entities.get_entity_data(weapon, 'stonehearth:combat:weapon_data')
   local melee_range_ideal, melee_range_max = stonehearth.combat:get_melee_range(entity, weapon_data, target)
   local engage_range_min = melee_range_ideal + 3
   local engage_range_max = engage_range_min + 3
   local distance

   ai:execute('stonehearth:chase_entity', { target = target, stop_distance = engage_range_min })
   distance = radiant.entities.distance_between(entity, target)

   if distance > engage_range_max then
      log:info('%s unable to get within maximum engagement range (%f) of %s. Giving up.', entity, engage_range_max, target)
      return
   end

   local context = EngageContext(entity)
   stonehearth.combat:engage(target, context)

   ai:execute('stonehearth:chase_entity', { target = target, stop_distance = melee_range_ideal })
   distance = radiant.entities.distance_between(entity, target)

   if distance > melee_range_max then
      log:info('%s unable to get within melee range (%f) of %s. Giving up.', entity, melee_range_max, target)
      return
   end
   
   ai:execute('stonehearth:combat:attack_melee_adjacent', { target = target, weapon = weapon })
end

return AttackMelee
