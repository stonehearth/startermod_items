local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HarvestResourceNodeAdjacent = class()

HarvestResourceNodeAdjacent.name = 'harvest resource node adj'
HarvestResourceNodeAdjacent.does = 'stonehearth:harvest_resource_node_adjacent'
HarvestResourceNodeAdjacent.args = {
   node = Entity      -- the entity to harvest
}
HarvestResourceNodeAdjacent.version = 2
HarvestResourceNodeAdjacent.priority = 1

function HarvestResourceNodeAdjacent:start()
   -- TODO: check to make sure we'll be next to the entity
end

function HarvestResourceNodeAdjacent:run(ai, entity, args)   
   local node = args.node
   local factory = node:get_component('stonehearth:resource_node')
   radiant.entities.turn_to_face(entity, node)

   if factory then
      repeat
            local effect = factory:get_harvester_effect()
            ai:execute('stonehearth:run_effect', { effect = effect})

            local location = radiant.entities.get_world_grid_location(entity)
            factory:spawn_resource(location)
      until not node:is_valid()   

   
      local description = factory:get_description()
      radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                             {entity = entity, description = description})
   end
end

return HarvestResourceNodeAdjacent
