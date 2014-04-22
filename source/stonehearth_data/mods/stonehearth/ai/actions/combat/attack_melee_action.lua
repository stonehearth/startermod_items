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

   local melee_range = stonehearth.combat:get_melee_range(entity, 'medium_1h_weapon', target)
   local engage_range_min = melee_range + 3
   local engage_range_max = engage_range_min + 3

   ai:execute('stonehearth:chase_entity', { target = target, stop_distance = engage_range_min })

   local distance = radiant.entities.distance_between(entity, target)

   if distance < engage_range_max then
      local context = EngageContext(entity)
      stonehearth.combat:engage(target, context)

      ai:execute('stonehearth:chase_entity', { target = target, stop_distance = melee_range })

      ai:execute('stonehearth:combat:attack_melee_adjacent', { target = target })
   else
      -- give up and go back to start thinking
   end
end

return AttackMelee
