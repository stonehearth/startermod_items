local DynamicScenarioCallHandler = class()

function DynamicScenarioCallHandler:force_spawn_scenario(session, request, scenario_uri) 
   stonehearth.dynamic_scenario:force_spawn_scenario(scenario_uri)
   return true;
end

return DynamicScenarioCallHandler
