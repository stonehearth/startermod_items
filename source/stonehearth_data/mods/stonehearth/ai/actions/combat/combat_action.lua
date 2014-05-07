local constants = require 'constants'
local log = radiant.log.create_logger('combat')

local Combat = class()
Combat.name = 'combat'
Combat.does = 'stonehearth:top'
Combat.args = {}
Combat.version = 2
Combat.priority = constants.priorities.top.COMBAT

function Combat:__init()
   self._enable_combat = radiant.util.get_config('enable_combat', false)
end

function Combat:start_thinking(ai, entity, args)
   if not self._enable_combat then
      return
   end

   self._ai = ai
   self._entity = entity
   self._think_output_set = false

   self:_find_target()

   if not self._think_output_set then
      self:_register_events()
   end
end

function Combat:stop_thinking(ai, entity, args)
   self:_unregister_events()
end

function Combat:_register_events()
   if not self._registered then
      -- TODO: replace slow_poll with a listen on target table changed
      radiant.events.listen(radiant, 'stonehearth:slow_poll', self, self._find_target)
      radiant.events.listen(self._entity, 'stonehearth:combat:engage', self, self._on_engage)
      self._registered = true
   end
end

function Combat:_unregister_events()
   if self._registered then
      radiant.events.unlisten(radiant, 'stonehearth:slow_poll', self, self._find_target)
      radiant.events.unlisten(self._entity, 'stonehearth:combat:engage', self, self._on_engage)
      self._registered = false
   end
end

function Combat:_on_engage(args)
   if self._think_output_set then
      return
   end
   
   self:_set_think_output(args.attacker)
end

function Combat:_set_think_output(enemy)
   assert(enemy ~= nil)
   if not self._think_output_set then
      self._enemy = enemy
      self._ai:set_think_output()
      self._think_output_set = true
   end
end

function Combat:_clear_think_output()
   if self._think_output_set then
      self._enemy = nil
      self._ai:clear_think_output()
      self._think_output_set = false
   end
end

function Combat:start(ai, entity, args)
   radiant.entities.set_posture(entity, 'combat')
end

function Combat:run(ai, entity, args)
   ai:execute('stonehearth:combat', { enemy = self._enemy })
end

function Combat:stop(ai, entity, args)
   radiant.entities.unset_posture(entity, 'combat')
end

function Combat:_find_target()
   local target_table = stonehearth.combat:get_target_table(self._entity, 'aggro')
   local target = target_table:get_top()

   -- if target table has no opinion, double check the assault events
   if target == nil then
      -- TODO: probably listen on both the assault events and target table
      local events = stonehearth.combat:get_assault_events(self._entity)
      local context = events[1] -- pick the assault that was initiated first

      if context ~= nil and context.attacker ~= nil and context.attacker:is_valid() then
         target = context.attacker
      end
   end

   if target ~= nil then
      self:_set_think_output(target)
      return radiant.events.UNLISTEN
   end
end

return Combat
