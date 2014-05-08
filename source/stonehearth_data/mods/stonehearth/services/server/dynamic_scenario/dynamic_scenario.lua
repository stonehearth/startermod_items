local DynamicScenario = class()

function DynamicScenario:__init(scenario)
  self._running = false
  self._scenario = scenario
end

function DynamicScenario:is_running()
  return self._running
end

function DynamicScenario:start()
  assert(not self._running)
  self._running = true
  radiant.events.listen(self._scenario, 'stonehearth:dynamic_scenario:finished', self, self._on_finished)

  self._scenario:start()
end

function DynamicScenario:_on_finished()
  self._running = false
  radiant.events.unlisten(self._scenario, 'stonehearth:dynamic_scenario:finished', self, self._on_finished)  
end

return DynamicScenario