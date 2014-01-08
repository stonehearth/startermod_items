local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local ChopTreeAdjacentAction = class()

ChopTreeAdjacentAction.name = 'chop tree adj'
ChopTreeAdjacentAction.does = 'stonehearth:chop_tree:adjacent'
ChopTreeAdjacentAction.args = {
   tree = Entity      -- the tree to chop
}
ChopTreeAdjacentAction.version = 2
ChopTreeAdjacentAction.priority = 1

function ChopTreeAdjacentAction:start()
   -- TODO: check to make sure we're next to the tree
end

function ChopTreeAdjacentAction:run(ai, entity, args)   
   local tree = args.tree

   repeat
      radiant.entities.turn_to_face(entity, tree)
      ai:execute('stonehearth:run_effect', { effect = 'chop'})
      
      local factory = tree:get_component('stonehearth:resource_node')
      if factory then
         local location = radiant.entities.get_world_grid_location(entity)
         factory:spawn_resource(location)
      end
   until not tree:is_valid()   
   radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                          {entity = entity, description = 'chop_tree'})
end

return ChopTreeAdjacentAction
