local DynamicScenario = class()

function DynamicScenario:initialize(scenario)
   self._sv._scenario = scenario
   self._sv._running = false
   scenario._sv.__scenario = self
end

function DynamicScenario:restore()
end

function DynamicScenario:can_spawn()
   return self._sv._scenario:can_spawn()
end

function DynamicScenario:is_running()
   return self._sv._running
end

function DynamicScenario:start()
   assert(not self._sv._running)
   self._sv._running = true
   radiant.events.listen_once(self._sv._scenario, 'stonehearth:dynamic_scenario:finished', self, self._on_finished)

   self._sv._scenario:start()
   self.__saved_variables:mark_changed()
end

function DynamicScenario:_on_finished()
   self._sv._running = false
   self.__saved_variables:mark_changed()
end

return DynamicScenario