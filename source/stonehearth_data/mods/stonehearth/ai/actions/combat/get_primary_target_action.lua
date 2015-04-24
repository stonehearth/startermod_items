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
GetPrimaryTarget.realtime = true

function GetPrimaryTarget:start_thinking(ai, entity, args)
   if self:_check_for_primary_target(ai, entity) then
      return
   end

   self._target_changed_listener = radiant.events.listen(entity, 'stonehearth:combat:primary_target_changed', function()
         if self:_check_for_primary_target(ai, entity) then
            self:_destroy_target_changed_listener()
         end
      end)
end

function GetPrimaryTarget:stop_thinking(ai, entity, args)
   self:_destroy_target_changed_listener()
end

function GetPrimaryTarget:_destroy_target_changed_listener()
   if self._target_changed_listener then
      self._target_changed_listener:destroy()
      self._target_changed_listener = nil
   end
end

function GetPrimaryTarget:_check_for_primary_target(ai, entity)
   local primary_target = stonehearth.combat:get_primary_target(entity)

   if primary_target and primary_target:is_valid() then
      ai:set_think_output({ target = primary_target })
      return primary_target
   else
      return nil
   end
end

return GetPrimaryTarget
