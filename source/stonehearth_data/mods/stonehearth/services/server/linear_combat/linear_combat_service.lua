LinearCombatService = class()

--[[
   This exists to mock up a faster, simpler, enemy-reaction driven version of combat
   Goblin bandits start to spawn instead of theives if a thief dies on a raid. 
]]

local time_till_first_spawn = '36h'
local spawn_interval = '24h'

function LinearCombatService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.num_escorts = 1
      self._sv.num_killed = 0
      self._sv.enabled = false
      self.__saved_variables:mark_changed()
   else
      if self._sv.enabled == true then
         radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
               self:_restore_spawn_timer()
            end)
      end
   end

   radiant.events.listen(self, 'stonehearth:goblin_killed', function() 
         self._sv.num_killed = self._sv.num_killed + 1 
      end)
end

function LinearCombatService:enable(enable)
   if enable == self._sv.enabled then
      return
   end

   self._sv.enabled = enable
   self.__saved_variables:mark_changed()

   if self._sv.enabled then
      self:_start_spawn_timer(time_till_first_spawn)
   else
      self:_stop_spawn_timer()
   end
end

function LinearCombatService:_restore_spawn_timer()
   local duration = self._sv.next_spawn_time and stonehearth.calendar:get_seconds_until(self._sv.next_spawn_time)

   if not duration or duration <= 0 then
      duration = spawn_interval
   end

   self:_start_spawn_timer(duration)
end

function LinearCombatService:_start_spawn_timer(duration)
   self:_stop_spawn_timer()

   self._spawn_timer = stonehearth.calendar:set_timer(duration, function()
         self:_stop_spawn_timer()
         self:_spawn()
      end
   )

   self._sv.next_spawn_time = self._spawn_timer:get_expire_time()
   self.__saved_variables:mark_changed()
end

function LinearCombatService:_stop_spawn_timer()
   if self._spawn_timer then
      self._spawn_timer:destroy()
      self._spawn_timer = nil
   end

   self._sv.next_spawn_time = nil
   self.__saved_variables:mark_changed()
end

function LinearCombatService:_spawn()
   if self._sv.enabled ~= nil and not self._sv.enabled then
      return
   end

   -- Start the next spawn timer first in case the scenario throws an exception.
   self:_start_spawn_timer(spawn_interval)

   -- If no goblin has been killed yet, spawn a thief. 
   if self._sv.num_killed < 1 then
      stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:goblin_thief')
   else 
      -- Otherwise, spawn a brigand
      stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:goblin_brigands', { num_escorts = self._sv.num_escorts })
      if self._sv.num_escorts < 3 then
         self._sv.num_escorts = self._sv.num_escorts + 1
         self.__saved_variables:mark_changed()
      end
   end
end

return LinearCombatService
