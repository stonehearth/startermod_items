--[[
   Tell a worker to collect food and resources from a plant. 
]]

local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local HarvestPlantsAction = class()
HarvestPlantsAction.name = 'harvest plant adjacent'
HarvestPlantsAction.does = 'stonehearth:harvest_plant_adjacent'
HarvestPlantsAction.args = {
   plant = Entity       -- the plant to harvest
}
HarvestPlantsAction.version = 2
HarvestPlantsAction.priority = 1

function HarvestPlantsAction:run(ai, entity, args)
   local plant = args.plant
   radiant.entities.turn_to_face(entity, plant)

   --Fiddle with the bush and pop the basket
   local factory = plant:get_component('stonehearth:renewable_resource_node')
   if factory then
      ai:execute('stonehearth:run_effect', { effect = 'fiddle' })
      local location = radiant.entities.get_world_grid_location(entity)
      local spawned_item = factory:spawn_resource(location)

      -- xxx: mucking with the personality compounent should probably be factored
      -- into its own action entirely (iff possible.... i don't even know if that's
      -- feasible) - tony
      --Log it in the personality component
      local spawned_item_name = spawned_item:get_component('unit_info'):get_display_name()
      local personality_component = entity:get_component('stonehearth:personality')
      personality_component:add_substitution('gather_target', spawned_item_name)
      personality_component:add_substitution_by_parameter('personality_gather_reaction', personality_component:get_personality(), 'stonehearth:settler_journals:gathering_supplies')

      radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                                  {entity = entity, description = 'gathering_supplies'})

   end   
end

return HarvestPlantsAction