local EngageContext = require 'services.server.combat.engage_context'
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local CreateEngageCallback = class()

CreateEngageCallback.name = 'create engage callback'
CreateEngageCallback.does = 'stonehearth:combat:create_engage_callback'
CreateEngageCallback.args = {
   target = Entity,
}
CreateEngageCallback.think_output = {
   callback = 'function',
}
CreateEngageCallback.version = 2
CreateEngageCallback.priority = 1
CreateEngageCallback.weight = 1

function CreateEngageCallback:start_thinking(ai, entity, args)
   local target = args.target
   local weapon = stonehearth.combat:get_melee_weapon(entity)

   if weapon == nil or not weapon:is_valid() then
      log:warning('%s has no weapon', entity)
      ai:abort('No weapon')
      return
   end

   local weapon_data = radiant.entities.get_entity_data(weapon, 'stonehearth:combat:weapon_data')
   assert(weapon_data)
   
   local engage_range_ideal, engage_range_max = stonehearth.combat:get_engage_range(entity, weapon_data, target)

   local engage_target_when_in_range = function()
      if not target:is_valid() then
         return
      end

      local distance = radiant.entities.distance_between(entity, target)

      if distance < engage_range_max then
         -- consider sending engage message just once?
         local context = EngageContext(entity)
         stonehearth.combat:engage(target, context)
      end
   end

   ai:set_think_output({ callback = engage_target_when_in_range })
end

return CreateEngageCallback
