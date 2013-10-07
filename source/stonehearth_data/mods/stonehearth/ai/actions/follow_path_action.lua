local FollowPathAction = class()

FollowPathAction.name = 'stonehearth.actions.follow_path'
FollowPathAction.does = 'stonehearth.follow_path'
FollowPathAction.priority = 1

function FollowPathAction:run(ai, entity, path)
   local speed = 1.0
   self._effect = radiant.effects.run_effect(entity, 'run')

   local dest_entity = path:get_destination()
   local destination_loc = dest_entity:get_component('mob'):get_world_grid_location()

   local arrived_fn = function()
      ai:resume()
   end
   self._mover = _radiant.sim.create_follow_path(entity, speed, path, 0, arrived_fn)
   ai:suspend()

   --if the item is no longer there or if it's moved, abort this
   if not dest_entity then
      ai:abort("Pathfinder's destination has disappeared! Help!")
   end

   --TODO: Haven't confirmed yet but sometimes I swear dest_entity is still null
   local curr_dest_location = dest_entity:get_component('mob'):get_world_grid_location()
   if curr_dest_location ~= destination_loc then
      ai:abort("Pathfinder's destination has moved! Help!")
   end

   self._effect:stop()
   self._effect = nil
end

function FollowPathAction:stop(ai, entity)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
   if self._mover then
      self._mover:stop()
      self._mover = nil
   end
end

return FollowPathAction