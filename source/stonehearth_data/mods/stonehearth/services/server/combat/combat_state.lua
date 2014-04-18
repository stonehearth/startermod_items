local log = radiant.log.create_logger('combat')

CombatState = class()

function CombatState:__init()
   self._cooldowns = {}
   self._assault_events = {}
end

-- duration is in milliseconds at game speed 1
function CombatState:start_cooldown(name, duration)
   local now = radiant.gamestate.now()
   self._cooldowns[name] = now + duration
end

function CombatState:in_cooldown(name)
   return self:get_cooldown_end_time(name) ~= nil
end

-- returns nil if the cooldown does not exist or has expired
-- note: start time is part of cooldown, while the end time is not
function CombatState:get_cooldown_end_time(name)
   local cooldown_end_time = self._cooldowns[name]

   if cooldown_end_time ~= nil then
      local now = radiant.gamestate.now()

      if now < cooldown_end_time then
         return cooldown_end_time
      end

      -- expired, clean up
      self._cooldowns[name] = nil
   end

   return nil
end

function CombatState:add_assault_event(context)
   table.insert(self._assault_events, context)
end

function CombatState:get_assault_events()
   -- clean up before returning the list
   self:_remove_expired_assault_events()

   return self._assault_events
end

-------------------------------------------------
-- private interface for the combat_service below
-------------------------------------------------

function CombatState:_remove_expired_cooldowns()
   local now = radiant.gamestate.now()
   local cooldowns = self._cooldowns

   -- safe to remove entries in a pairs loop: http://lua-users.org/wiki/TablesTutorial
   for name, cooldown_end_time in pairs(cooldowns) do
      if now >= cooldown_end_time then
         cooldowns[name] = nil
      end
   end
end

function CombatState:_remove_expired_assault_events()
   local now = radiant.gamestate.now()
   local events = self._assault_events

   -- remove in reverse so we don't have to worry about iteration order
   for i = #events, 1, -1 do
      local context = events[i]
      if context.impact_time < now then
         table.remove(events, i)
      end
   end
end

return CombatState
