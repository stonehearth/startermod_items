LinearCombatService = class()

--[[
   This exists to mock up a faster, simpler, enemy-reaction driven version of combat
   Goblin bandits start to spawn instead of theives if a thief dies on a raid. 
]]

local time_till_first_spawn = '6h'
local spawn_interval = '24h'

function LinearCombatService:initialize()
   self._sv = self.__saved_variables:get_data()
   self._num_escorts = 1
   if not self._sv._init then
      self._sv.init = true
      self._timer = stonehearth.calendar:set_timer(time_till_first_spawn, function()
          self:_first_spawn() end)
      self._sv.expire_time = self._timer:get_expire_time()
      self._sv.first_spawn = false
      self._sv.num_killed = 0
   else
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         self:_create_load_timer()
         return radiant.events.UNLISTEN
      end)
   end

   radiant.events.listen(self, 'stonehearth:goblin_killed', function() 
      self._sv.num_killed = self._sv.num_killed + 1 
   end)
end

function LinearCombatService:_create_load_timer()
   if self._sv.expire_time then
      --We're still growing
      local duration = self._sv.expire_time - stonehearth.calendar:get_elapsed_time()
      self._timer = stonehearth.calendar:set_timer(duration, function()
         if not self._sv.first_spawn then
            --We want to call _first_spawn because it hasn't been called yet
            self:_first_spawn()
         else
            --We were loaded during a spawn interval
            self:_subsequent_spawn()
            self._timer = stonehearth.calendar:set_interval(spawn_interval, self._subsequent_spawn)
            self._sv.expire_time = self._timer:get_expire_time()
         end
      end)
   end
end

function LinearCombatService:_first_spawn()
   self._sv.first_spawn = true

   --After the first spawn, spawn something every interval mentioned above
   self._timer = stonehearth.calendar:set_interval(spawn_interval, function() self:_subsequent_spawn() end)
   self._sv.expire_time = self._timer:get_expire_time()

   stonehearth.dynamic_scenario:force_spawn_scenario('Goblin Thief')
end

function LinearCombatService:_subsequent_spawn()
   if self._timer then
      self._sv.expire_time = self._timer:get_expire_time()
   end

   --If no goblin has been killed yet, spawn a thief. 
   if self._sv.num_killed < 1 then
      stonehearth.dynamic_scenario:force_spawn_scenario('Goblin Thief')
   else 
      --Otherwise, spawn a brigand
      stonehearth.dynamic_scenario:force_spawn_scenario('Goblin Brigands', { num_escorts = self._num_escorts })
      if self._num_escorts < 3 then
         self._num_escorts = self._num_escorts + 1
      end
   end
end


return LinearCombatService