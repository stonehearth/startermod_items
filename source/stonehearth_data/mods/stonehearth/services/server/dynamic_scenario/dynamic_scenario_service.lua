local log = radiant.log.create_logger('dynamic_scenario_service')
local rng = _radiant.csg.get_default_rng()

local DynamicScenarioService = class()

function DynamicScenarioService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv.running_scenarios then
      self._sv.running_scenarios = {}
      self._sv.last_spawn_times = {}
   end
   self:_parse_scenario_index()
end


function DynamicScenarioService:force_spawn_scenario(scenario_uri)
   local scenario = self._scenarios_by_uri[scenario_uri]
   assert(type(scenario) == 'table')

   local new_scenario = self:_create_scenario(scenario)
   if not new_scenario:can_spawn() then
      radiant.destroy_controller(new_scenario)
      return
   end
   new_scenario:start()
   table.insert(self._sv.running_scenarios, new_scenario)
   return new_scenario
end


-- At this point, the DM wants to spawn a scenario of kind 'scenario_type'.  Look
-- through our list of scenarios for that type, and see if we find one that we
-- can spawn.
function DynamicScenarioService:try_spawn_scenario(scenario_type, pace_keepers)
   local pace_keeper = pace_keepers[scenario_type]

   -- if we have no scenarios of this type, bail
   local scenarios = self._scenarios_by_type[scenario_type]
   if not scenarios then
      return
   end

   -- if there's no pace_keeper or it's not willing to spawn this scenario
   -- type yet, bail.
   if not pace_keeper or
      not pace_keeper:willing_to_spawn() then
      return
   end

   local best_scenario, best_info
   for _, info in pairs(scenarios) do
      -- see if we pass the rarity check...
      local rarity = self:_rarity_to_value(info.properties.rarity)
      if rng:get_int(1, rarity) == 1 then
         -- yay!  now create the scenario and see if it's willing to
         -- spawn at this time.
         local scenario = self:_create_scenario(info.properties)
         if scenario:can_spawn() then
            -- sweet.  if this is the least recently activated of all the
            -- scenarios that can run, use this one.
            info.buildup = info.buildup + 1
            if not best_info or info.buildup > best_info.buildup then
               if best_scenario then
                  radiant.destroy_controller(best_scenario)
               end
               best_info = info
               best_scenario = scenario
            end
         end

         if scenario ~= best_scenario then
            radiant.destroy_controller(scenario)
         end
      end
   end

   -- bail if we couldn't find one to activate.
   if not best_scenario then
      return
   end

   -- Let the pacekeeper scenario know that we've spawned a scenario.
   pace_keeper:clear_buildup()
   pace_keeper:spawning_scenario(best_scenario)

   log:spam('Spawning new %s scenario %s', scenario_type, best_info.properties.uri)
   
   best_info.buildup = 0
   best_scenario:start()

   table.insert(self._sv.running_scenarios, best_scenario)
   self.__saved_variables:mark_changed()
end


function DynamicScenarioService:_rarity_to_value(rarity)
   if rarity == 'common' then
      return 2 -- 50% chance of not being culled.
   elseif rarity == 'uncommon' then
      return 5 -- 20% chance of not being culled.
   elseif rarity == 'rare' then
      return 20 -- 5% chance of not being culled.
   end
   assert(false)
end


function DynamicScenarioService:num_running_scenarios()
   local num_running = 0

   for i = #self._sv.running_scenarios, 1, -1 do
      local scenario = self._sv.running_scenarios[i]
      if scenario:is_running() then
         num_running = num_running + 1
      else 
         local s = table.remove(self._sv.running_scenarios, i)
         radiant.destroy_controller(s)
      end
   end
   self.__saved_variables:mark_changed()

   return num_running
end


function DynamicScenarioService:create_new_game()
end


function DynamicScenarioService:_parse_scenario_index()
   self._scenarios_by_uri = {}
   self._scenarios_by_type = {}

   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')

   for _, file in pairs(scenario_index.dynamic.scenarios) do
      local properties = radiant.resources.load_json(file)
      self._scenarios_by_uri[file] = properties
      
      local scenario_type = properties.scenario_type
      if self._scenarios_by_type[scenario_type] == nil then
         self._scenarios_by_type[scenario_type] = {}
      end
      local scenario_data = {  
         uri = properties.uri,
         properties = properties,
         buildup = 0
      }
      table.insert(self._scenarios_by_type[scenario_type], scenario_data)
   end
end

function DynamicScenarioService:_create_scenario(scenario)
   assert(type(scenario) == 'table')
   assert(type(scenario.scenario_type) == 'string')

   if scenario.scenario_type == 'settlement' then
      return radiant.create_controller('stonehearth:settlement_scenario_wrapper', scenario)
   end

   -- use the generic wrapper for everything else
   local s = radiant.create_controller(scenario.uri)
   return radiant.create_controller('stonehearth:generic_scenario_wrapper', s)
end

return DynamicScenarioService
