local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local DropCarryingNow = class()

DropCarryingNow.name = 'drop carrying now'
DropCarryingNow.does = 'stonehearth:drop_carrying_now'
DropCarryingNow.args = { }
DropCarryingNow.version = 2
DropCarryingNow.priority = 1

function DropCarryingNow:start_thinking(ai, entity, args)
   ai.CURRENT.carrying = nil
   ai:set_think_output()
end

function DropCarryingNow:run(ai, entity, args)
   if not radiant.entities.get_carrying(entity) then
      return
   end

   local mob = entity:add_component('mob')
   local facing = mob:get_facing()
   local location = mob:get_world_grid_location()

   local front = self:_get_front_grid_location(location, facing)

   if _physics:is_standable(front) then
      self._drop_location = front
   else
      -- drop at our feet is front is blocked
      self._drop_location = location
   end

   radiant.entities.turn_to_face(entity, front)
   ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })

   -- place the item now, don't wait for the compound action (if it exists) to terminate
   self:stop(ai, entity, args)
end

-- if the animation is interrupted, complete the drop since we've already let go of the item!
function DropCarryingNow:stop(ai, entity, args)
   if self._drop_location then
      radiant.entities.drop_carrying_on_ground(entity, self._drop_location)
      self._drop_location = nil
   end
end

function DropCarryingNow:_get_front_grid_location(location, facing)
   -- align to one of the four non-diagonal adjacent voxels
   facing = radiant.math.round(facing / 90) * 90

   -- 0 degress is along the negative z axis
   local offset = radiant.math.rotate_about_y_axis(-Point3f.unit_z, facing):to_closest_int()
   local front = location + offset
   return front
end

return DropCarryingNow
