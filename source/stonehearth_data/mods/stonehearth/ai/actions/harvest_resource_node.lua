local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HarvestResourceNodeAction = class()

HarvestResourceNodeAction.name = 'harvest resource node'
HarvestResourceNodeAction.does = 'stonehearth:harvest_resource_node'
HarvestResourceNodeAction.args = {
   node = Entity      -- the entity to harvest
}
HarvestResourceNodeAction.version = 2
HarvestResourceNodeAction.priority = 1

function HarvestResourceNodeAction:start(ai, entity, args)
   ai:set_status_text('harvesting ' .. radiant.entities.get_name(args.node))
end

local ai = stonehearth.ai
return ai:create_compound_action(HarvestResourceNodeAction)
         :when( function (ai) return ai.CURRENT.carrying == nil end )
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.node })
         :execute('stonehearth:harvest_resource_node_adjacent', { node = ai.ARGS.node })
         :execute('stonehearth:trigger_event', {
            source = stonehearth.personality,
            event_name = 'stonehearth:journal_event',
            event_args = {
               entity = ai.ENTITY,
               description = 'harvest_entity',
            },
         })
