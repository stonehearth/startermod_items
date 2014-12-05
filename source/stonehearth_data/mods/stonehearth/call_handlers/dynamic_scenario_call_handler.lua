local DynamicScenarioCallHandler = class()

function DynamicScenarioCallHandler:spawn_scenario(session, request, scenario_uri) 
   stonehearth.dynamic_scenario:force_spawn_scenario(scenario_uri)
   return true;
end

function DynamicScenarioCallHandler:cl_spawn_scenario(session, request, scenario_uri) 
   _radiant.call('stonehearth:spawn_scenario', scenario_uri)
   return true;
end

return DynamicScenarioCallHandler
