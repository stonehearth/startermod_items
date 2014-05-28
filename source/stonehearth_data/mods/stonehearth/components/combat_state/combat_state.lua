local log = radiant.log.create_logger('combat')

CombatState = class()

function CombatState:__init()
   -- attacks in progress are not saved, so neither are their assault events
   self._assault_events = {}
end

function CombatState:initialize()
   self.__saved_variables = radiant.create_datastore()
   self._sv = self.__saved_variables:get_data()

   self._sv.cooldowns = {}
   self._sv.is_panicking = false
end

function CombatState:restore(datastore)
   self.__saved_variables = datastore
   self._sv = self.__saved_variables:get_data()
end

-- duration is in milliseconds at game speed 1
function CombatState:start_cooldown(name, duration)
   local now = radiant.gamestate.now()
   self._sv.cooldowns[name] = now + duration
   self.__saved_variables:mark_changed()
end

function CombatState:in_cooldown(name)
   return self:get_cooldown_end_time(name) ~= nil
end

-- returns nil if the cooldown does not exist or has expired
-- note: start time is part of cooldown, while the end time is not
function CombatState:get_cooldown_end_time(name)
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

function CombatState:add_assault_event(context)
   table.insert(self._assault_events, context)
   self.__saved_variables:mark_changed()
end

function CombatState:get_assault_events()
   -- clean up before returning the list
   self:remove_expired_assault_events()

   return self._assault_events
end

function CombatState:set_panicking(is_panicking)
   self._sv.is_panicking = is_panicking
   self.__saved_variables:mark_changed()
end

function CombatState:is_panicking()
   return self._sv.is_panicking
end

function CombatState:remove_expired_cooldowns()
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

function CombatState:remove_expired_assault_events()
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

return CombatState
