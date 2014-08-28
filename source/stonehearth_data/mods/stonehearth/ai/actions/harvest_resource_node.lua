local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HarvestResourceNode = class()

HarvestResourceNode.name = 'harvest resource node'
HarvestResourceNode.does = 'stonehearth:harvest_resource_node'
HarvestResourceNode.args = {
   node = Entity      -- the entity to harvest
}
HarvestResourceNode.version = 2
HarvestResourceNode.priority = 1

function HarvestResourceNode:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      local resource_node_component = args.node:get_component('stonehearth:resource_node')
      if resource_node_component and resource_node_component:is_harvestable() then
         ai:set_think_output()
      end
   end
end

function HarvestResourceNode:start(ai, entity, args)
   ai:set_status_text('harvesting ' .. radiant.entities.get_name(args.node))
end

local resource_radius = 0.5 -- distance from the center of a voxel to the edge
local entity_reach = 1.0   -- read this from entity_data.stonehearth:entity_reach
local tool_reach = 0.75     -- read this from entity_data.stonehearth:weapon_data.reach
local harvest_range = entity_reach + tool_reach + resource_radius

local ai = stonehearth.ai
return ai:create_compound_action(HarvestResourceNode)
         :execute('stonehearth:goto_entity', {
            entity = ai.ARGS.node,
            stop_distance = harvest_range,
         })
         :execute('stonehearth:harvest_resource_node_adjacent', {
            node = ai.ARGS.node
         })
         :execute('stonehearth:trigger_event', {
            source = stonehearth.personality,
            event_name = 'stonehearth:journal_event',
            event_args = {
               entity = ai.ENTITY,
               description = 'harvest_entity',
            },
         })
