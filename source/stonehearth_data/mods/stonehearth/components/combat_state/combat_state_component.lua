local CombatStateComponent = class()

function CombatStateComponent:__init()
   -- attacks in progress are not saved, so neither are their assault events
   self._assault_events = {}
end

function CombatStateComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.cooldowns = {}
      self._sv.is_panicking = false
      self._sv.initialized = true
   end

   -- ten second or minute poll is sufficient
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._clean_combat_state)
end

function CombatStateComponent:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._clean_combat_state)
end

-- duration is in milliseconds at game speed 1
function CombatStateComponent:start_cooldown(name, duration)
   local now = radiant.gamestate.now()
   self._sv.cooldowns[name] = now + duration
   self.__saved_variables:mark_changed()
end

function CombatStateComponent:in_cooldown(name)
   return self:get_cooldown_end_time(name) ~= nil
end

-- returns nil if the cooldown does not exist or has expired
-- note: start time is part of cooldown, while the end time is not
function CombatStateComponent:get_cooldown_end_time(name)
   local cooldown_end_time = self._sv.cooldowns[name]

   if cooldown_end_time ~= nil then
      local now = radiant.gamestate.now()

      if now < cooldown_end_time then
         return cooldown_end_time
      end

      -- expired, clean up
      self._sv.cooldowns[name] = nil
   end

   return nil
end

function CombatStateComponent:add_assault_event(context)
   table.insert(self._assault_events, context)
   self.__saved_variables:mark_changed()
end

function CombatStateComponent:get_assault_events()
   -- clean up before returning the list
   self:_remove_expired_assault_events()

   return self._assault_events
end

function CombatStateComponent:set_panicking(is_panicking)
   self._sv.is_panicking = is_panicking
   self.__saved_variables:mark_changed()
end

function CombatStateComponent:is_panicking()
   return self._sv.is_panicking
end

function CombatStateComponent:_clean_combat_state()
   self:_remove_expired_cooldowns()
   self:_remove_expired_assault_events()
end

function CombatStateComponent:_remove_expired_cooldowns()
   local now = radiant.gamestate.now()
   local cooldowns = self._sv.cooldowns
   local changed = false

   -- safe to remove entries in a pairs loop: http://lua-users.org/wiki/TablesTutorial
   for name, cooldown_end_time in pairs(cooldowns) do
      if now >= cooldown_end_time then
         cooldowns[name] = nil
         changed = true
      end
   end

   if changed then
      self.__saved_variables:mark_changed()
   end
end

function CombatStateComponent:_remove_expired_assault_events()
   local now = radiant.gamestate.now()
   local events = self._assault_events
   local changed = false

   -- remove in reverse so we don't have to worry about iteration order
   for i = #events, 1, -1 do
      local context = events[i]
      if context.impact_time < now then
         table.remove(events, i)
         changed = true
      end
   end

   if changed then
      self.__saved_variables:mark_changed()
   end
end

return CombatStateComponent
