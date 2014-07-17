local log = radiant.log.create_logger('worker_defense')

WorkerDefense = class()

function WorkerDefense:__init()
   self._eligible_professions = {}
   self._eligible_professions['stonehearth:professions:worker'] = true
   self._eligible_professions['stonehearth:professions:farmer'] = true
   self._eligible_professions['stonehearth:professions:carpenter'] = true
   self._eligible_professions['stonehearth:professions:weaver'] = true
end

function WorkerDefense:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.worker_combat_enabled = {}
   else
      for player_id, enabled in pairs(self._sv.worker_combat_enabled) do
         if enabled then
            self:enable_worker_combat(player_id)
         end
      end
   end
end

function WorkerDefense:destroy()
end

function WorkerDefense:worker_combat_is_enabled(player_id)
   return self._sv.worker_combat_enabled[player_id]
end

function WorkerDefense:enable_worker_combat(player_id)
   local population = stonehearth.population:get_population(player_id)
   local citizens = population:get_citizens()
   
   for _, citizen in pairs(citizens) do
      if self:_performs_worker_defense(citizen) then
         local combat_state = citizen:add_component('stonehearth:combat_state')
         combat_state:set_stance('aggressive')

         -- pop a ! over the citizen's head
         radiant.entities.think(citizen, '/stonehearth/data/effects/thoughts/alert', stonehearth.constants.think_priorities.ALERT)
      end
   end

   self._sv.worker_combat_enabled[player_id] = true
   self.__saved_variables:mark_changed()
end

function WorkerDefense:disable_worker_combat(player_id)
   local population = stonehearth.population:get_population(player_id)
   local citizens = population:get_citizens()

   for _, citizen in pairs(citizens) do
      local combat_state = citizen:add_component('stonehearth:combat_state')
      combat_state:set_stance('passive')

      -- remove the ! from above the citizen's head
      radiant.entities.unthink(citizen, '/stonehearth/data/effects/thoughts/alert',  stonehearth.constants.think_priorities.ALERT)
   end 

   self._sv.worker_combat_enabled[player_id] = false
   self.__saved_variables:mark_changed()
end

function WorkerDefense:_performs_worker_defense(entity)
   local profession_component = entity:get_component('stonehearth:profession')
   local profession

   if not profession_component then
      return false
   end

   profession = profession_component:get_profession_uri()
   return self._eligible_professions[profession]
end

return WorkerDefense
