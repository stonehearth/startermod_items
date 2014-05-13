local DynamicScenario = class()

function DynamicScenario:__init(scenario_class, scenario_script_path, datastore)
  self.__saved_variables = datastore
  self._sv = datastore:get_data()

  if not self._sv._scenario_datastore then
    self._sv._scenario_datastore = radiant.create_datastore()
    self._sv._running = false
    self._sv._scenario_script_path = scenario_script_path
    self.__saved_variables:mark_changed()
  end
  self._sv._scenario = scenario_class(self._sv._scenario_datastore)

end

function DynamicScenario:is_running()
  return self._sv._running
end

function DynamicScenario:start()
  assert(not self._running)
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