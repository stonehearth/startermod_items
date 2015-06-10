local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HarvestPlant = class()

HarvestPlant.name = 'harvest plant'
HarvestPlant.does = 'stonehearth:harvest_plant'
HarvestPlant.args = {
   plant = Entity      -- the plant to chop
}
HarvestPlant.version = 2
HarvestPlant.priority = 1

function HarvestPlant:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      local resource_node_component = args.plant:get_component('stonehearth:renewable_resource_node')
      if resource_node_component and resource_node_component:is_harvestable() then
         ai:set_think_output()
      end
   end
end

function HarvestPlant:start(ai, entity, args)
   ai:set_status_text('harvesting ' .. radiant.entities.get_name(args.plant))
end

local ai = stonehearth.ai
return ai:create_compound_action(HarvestPlant)
         :execute('stonehearth:drop_carrying_now', {})
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.plant })
         :execute('stonehearth:harvest_plant_adjacent', { plant = ai.ARGS.plant })
