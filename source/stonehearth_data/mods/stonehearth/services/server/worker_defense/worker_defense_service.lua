local log = radiant.log.create_logger('worker_defense')

WorkerDefense = class()

function WorkerDefense:__init()
   self._eligible_professions = {}
   self._eligible_professions['stonehearth:professions:worker'] = true
   self._eligible_professions['stonehearth:professions:farmer'] = true
   self._eligible_professions['stonehearth:professions:carpenter'] = true
   self._eligible_professions['stonehearth:professions:weaver'] = true

   self._injected_ai_map = {}
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
      if self:_has_eligible_profession(citizen) then
         -- inject the combat ai
         local ai_handles = self:_inject_worker_combat_ai(citizen)

         -- save the ai handles so we can remove them later
         local injected_ais = self:_get_injected_ais(player_id)
         injected_ais[citizen:get_id()] = ai_handles

         -- TODO: should we make the ai permenant and just toggle the stance?
         local combat_state = citizen:add_component('stonehearth:combat_state')
         combat_state:set_stance('aggressive')

         -- pop a ! over the citizen's head
         radiant.entities.think(citizen, '/stonehearth/data/effects/thoughts/alert',  stonehearth.constants.think_priorities.ALERT)
      end
   end

   self._sv.worker_combat_enabled[player_id] = true
   self.__saved_variables:mark_changed()
end

function WorkerDefense:disable_worker_combat(player_id)
   local population = stonehearth.population:get_population(player_id)
   local citizens = population:get_citizens()
   local injected_ais = self:_get_injected_ais(player_id)

   for id, ai_handles in pairs(injected_ais) do
      local entity = radiant.entities.get_entity(id)
      if entity then
         for _, ai in pairs(ai_handles) do
            ai:destroy()
         end

         -- might need to save and restore prior state later
         local combat_state = entity:add_component('stonehearth:combat_state')
         combat_state:set_stance('passive')
      end
   end

   for _, citizen in pairs(citizens) do
      -- remove the ! from above the citizen's head
      radiant.entities.unthink(citizen, '/stonehearth/data/effects/thoughts/alert',  stonehearth.constants.think_priorities.ALERT)
   end 

   self._sv.worker_combat_enabled[player_id] = false
   self.__saved_variables:mark_changed()
end

function WorkerDefense:_get_injected_ais(player_id)
   local injected_ais = self._injected_ai_map[player_id]

   if not injected_ais then
      injected_ais = {}
      self._injected_ai_map[player_id] = injected_ais
   end

   return injected_ais
end

function WorkerDefense:_inject_worker_combat_ai(entity)
   return {
      stonehearth.ai:inject_ai(entity, { observers = { "stonehearth:observers:enemy_observer" }}),
      stonehearth.ai:inject_ai(entity, { observers = { "stonehearth:observers:ally_defense" }})
   }
end

function WorkerDefense:_has_eligible_profession(entity)
   local profession_component = entity:get_component('stonehearth:profession')
   local profession

   if not profession_component then
      return false
   end

   profession = profession_component:get_profession_uri()
   return self._eligible_professions[profession]
end

return WorkerDefense
