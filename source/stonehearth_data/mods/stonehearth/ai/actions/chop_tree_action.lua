local ChopTreeAction = class()

ChopTreeAction.does = 'stonehearth:chop_tree'
ChopTreeAction.priority = 1

function ChopTreeAction:run(ai, entity, path)
   local tree = path:get_destination()
   
   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, tree)
   ai:execute('stonehearth:run_effect', 'chop')
   
   local factory = tree:get_component('stonehearth:resource_node')
   if factory then
      local location = radiant.entities.get_world_grid_location(entity)
      factory:spawn_resource(location)
   end
end

return ChopTreeAction

