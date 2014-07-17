local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local GetPrimaryTarget = class()

GetPrimaryTarget.name = 'get primary target'
GetPrimaryTarget.does = 'stonehearth:combat:get_primary_target'
GetPrimaryTarget.args = {}
GetPrimaryTarget.think_output = {
   target = Entity,
}
GetPrimaryTarget.version = 2
GetPrimaryTarget.priority = 1

function GetPrimaryTarget:start_thinking(ai, entity, args)
   local primary_target = stonehearth.combat:get_primary_target(entity)

   if primary_target and primary_target:is_valid() then
      ai:set_think_output({ target = primary_target })
   end
end

return GetPrimaryTarget
