local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local FindTarget = class()
FindTarget.name = 'find target'
FindTarget.does = 'stonehearth:combat:find_target'
FindTarget.args = {}
FindTarget.think_output = {
   target = Entity,
}
FindTarget.version = 2
FindTarget.priority = 1

function FindTarget:__init()
   self._enable_combat = radiant.util.get_config('enable_combat', false)
end

function FindTarget:start_thinking(ai, entity, args)
   if not self._enable_combat then
      return
   end

   self._ai = ai
   self._entity = entity
   self._registered = false

   local target = self:_get_target()

   if target ~= nil and target:is_valid() then
      ai:set_think_output({ target = target })
      return
   end

   self:_register_events()
end

function FindTarget:stop_thinking(ai, entity, args)
   self:_unregister_events()
   self._ai = nil
   self._entity = nil
end

function FindTarget:_register_events()
   if not self._registered then
      -- TODO: replace slow_poll with a listen on target table changed
      -- TODO: have engage add the engager to the target table
      radiant.events.listen(radiant, 'stonehearth:slow_poll', self, self._find_target)
      radiant.events.listen(self._entity, 'stonehearth:combat:engage', self, self._on_engage)
      self._registered = true
   end
end

function FindTarget:_unregister_events()
   if self._registered then
      radiant.events.unlisten(radiant, 'stonehearth:slow_poll', self, self._find_target)
      radiant.events.unlisten(self._entity, 'stonehearth:combat:engage', self, self._on_engage)
      self._registered = false
   end
end

function FindTarget:_on_engage(args)
   local target = args.attacker

   if target ~= nil and target:is_valid() then
      self._ai:set_think_output({ target = args.attacker })
      self:_unregister_events()
   end
end

function FindTarget:_get_target()
   local target_table = stonehearth.combat:get_target_table(self._entity, 'aggro')
   local target = target_table:get_top()

   -- if target table has no opinion, double check the assault events
   if target == nil or not target:is_valid() then
      -- TODO: probably listen on both the assault events and target table
      local events = stonehearth.combat:get_assault_events(self._entity)
      local context = events[1] -- pick the assault that was initiated first

      if context ~= nil and context.attacker ~= nil and context.attacker:is_valid() then
         target = context.attacker
      end
   end

   return target
end

function FindTarget:_find_target()
   local target = self:_get_target()

   if target ~= nil and target:is_valid() then
      self._ai:set_think_output({ target = target })
      self:_unregister_events()
   end
end

return FindTarget
