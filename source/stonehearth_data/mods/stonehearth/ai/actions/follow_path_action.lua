local FollowPathAction = class()

FollowPathAction.name = 'follow path'
FollowPathAction.does = 'stonehearth:follow_path'
FollowPathAction.priority = 1

function FollowPathAction:run(ai, entity, path, effect_name)
   local speed = radiant.entities.get_attribute(entity, 'speed')
   
   if speed == nil then
      speed = 100
   end
   speed = speed / 100

   if not effect_name then
      effect_name = 'run'
   end

   if not self._effect then
      self._effect = radiant.effects.run_effect(entity, effect_name)
   end

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

   local mob_component = dest_entity:get_component('mob')
   if not mob_component or mob_component:get_world_grid_location() ~= destination_loc then
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