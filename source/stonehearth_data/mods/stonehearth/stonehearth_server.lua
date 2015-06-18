local StonehearthCommon = require 'stonehearth.stonehearth_common'
local LootTable = require 'stonehearth.lib.loot_table.loot_table'

stonehearth = {
   constants = require 'constants'
}

local service_creation_order = {
   'threads',
   'ai',
   'events',
   'calendar',
   'game_speed',
   'tasks',
   'terrain',
   'score',
   'combat',
   'substitution',
   'personality',
   'inventory',
   'population',
   'town',
   'job',
   'player',
   'spawn_region_finder',
   'dm',
   'static_scenario',
   'dynamic_scenario',
   'world_generation',
   'build',
   'game_master',
   'analytics',
   'mining',
   'hydrology',
   'swimming',
   'town_patrol',
   'bulletin_board',
   'linear_combat',
   'farming',
   'trapping',
   'shepherd',
   'shop',
   'tutorial',
   'unit_control',
}

local function create_service(name)
   local path = string.format('services.server.%s.%s_service', name, name)
   local service = require(path)()

   local saved_variables = stonehearth._sv[name]
   if not saved_variables then
      saved_variables = radiant.create_datastore()
      stonehearth._sv[name] = saved_variables
   end
   service.__saved_variables = saved_variables
   service._sv = saved_variables:get_data()
   saved_variables:set_controller(service)
   service:initialize()
   stonehearth[name] = service
end

local function run_on_destroy_effect(entity)
   --If the entity has specified to run an effect, run it.
   local on_destroy = radiant.entities.get_entity_data(entity, 'stonehearth:on_destroy')
   if on_destroy ~= nil and on_destroy.effect ~= nil then
   
      local proxy_entity = radiant.entities.create_entity(nil, { debug_text = 'death effect' })
      local location = radiant.entities.get_location_aligned(entity)
      radiant.terrain.place_entity_at_exact_location(proxy_entity, location)

      local effect = radiant.effects.run_effect(proxy_entity, on_destroy.effect)
      effect:set_finished_cb(
         function()
            radiant.entities.destroy_entity(proxy_entity)
         end
      )
   end
end

--Review Comment: (@tony) I realize this fn is about generating loot from entity data, and
--loot_drops_component is about generating loot from a table added at runtime, but maybe it
--would make sense to consolidate the loot generation somewhere, maybe in entities.lua? 
--I could add a drop_loot function. (--sdee)
local function spawn_loot(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   if location then
      local loot_table = radiant.entities.get_entity_data(entity, 'stonehearth:destroyed_loot_table')
      if loot_table then
         local items = LootTable(loot_table)
                           :roll_loot()
                           
         local owner_id = radiant.entities.get_player_id(entity)
         local spawned_entities = radiant.entities.spawn_items(items, location, 1, 3, { owner = owner_id })
      
         --Add a loot command to each of the spawned items
         for id, entity in pairs(spawned_entities) do 
            local target = entity
            local entity_forms = entity:get_component('stonehearth:entity_forms')
            if entity_forms then
               target = entity_forms:get_iconic_entity()
            end
            local command_component = target:add_component('stonehearth:commands')
            command_component:add_command('/stonehearth/data/commands/loot_item')
         end
      end
   end
end

local function cleanup_entity(entity)
   StonehearthCommon.destroy_child_entities(entity)
   StonehearthCommon.destroy_entity_forms(entity)
   run_on_destroy_effect(entity)
   spawn_loot(entity)
end

radiant.events.listen(stonehearth, 'radiant:init', function()
      -- global config
      radiant.terrain.set_config_file('stonehearth:terrain_block_config', true)

      -- now create all the services
      stonehearth._sv = stonehearth.__saved_variables:get_data()
      for _, name in ipairs(service_creation_order) do
         create_service(name)
      end

      radiant.events.listen(radiant, 'radiant:entity:pre_destroy', function(e)
            xpcall(function()
                  cleanup_entity(e.entity)
               end,
               radiant.report_traceback
            )
         end)
   end)

radiant.events.listen(stonehearth, 'radiant:new_game', function()
      -- stop the calendar service until the user has a chance to
      -- work their way into the actual game.
      stonehearth.calendar:stop()
   end)

return stonehearth
