local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local ChopTreeAction = class()

ChopTreeAction.name = 'chop tree'
ChopTreeAction.does = 'stonehearth:chop_tree'
ChopTreeAction.args = {
   Entity      -- the tree to chop
}
ChopTreeAction.version = 2
ChopTreeAction.priority = 1

function ChopTreeAction:run(ai, entity, path)
   local tree = path:get_destination()

   -- the tree could be nil here if something happened to it before
   -- we got a chance to chop it down.  nothing terribly bad would happen
   -- if we continued from here... the coroutine would crash and the
   -- entity would find something else to do.  however, this is a fairly
   -- common occurance, so just abort gracefully instead.
   if not tree then
      ai:abort('tree is nil in stonehearth:chop_tree') 
   end

   ai:execute('stonehearth:follow_path', path)

   repeat
      radiant.entities.turn_to_face(entity, tree)
      ai:execute('stonehearth:run_effect', 'chop')
      
      local factory = tree:get_component('stonehearth:resource_node')
      if factory then
         local location = radiant.entities.get_world_grid_location(entity)
         factory:spawn_resource(location)
      end
   until not tree:is_valid()
   
   radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                          {entity = entity, description = 'chop_tree'})
end

return ChopTreeAction

