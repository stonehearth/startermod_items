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

   local events = stonehearth.combat:get_assault_events(entity)
   local context = events[1] -- just pick the first attacker for now
   if context ~= nil and context.attacker ~= nil then
      self:_set_think_output(context.attacker)
      return
   end

   self:_register_events()
end

function Combat:stop_thinking(ai, entity, args)
   self:_unregister_events()
end

function Combat:_register_events()
   if not self._registered then
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
   -- TODO: figure out where to unset the combat posture
   -- weapon might blink during the next start_thinking
   radiant.entities.unset_posture(entity, 'combat')
end

function Combat:_find_target()
   local target = self:_get_target()

   if target ~= nil then
      self:_set_think_output(target)
   else
      self:_clear_think_output()
   end
end

function Combat:_get_target()
   local target_table = stonehearth.combat:get_target_table(self._entity, 'aggro')
   local target = target_table:get_top()
   return target
end

return Combat
