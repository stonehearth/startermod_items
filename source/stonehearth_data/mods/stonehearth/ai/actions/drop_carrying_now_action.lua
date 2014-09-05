local Point3 = _radiant.csg.Point3
local DropCarryingNow = class()

DropCarryingNow.name = 'drop carrying now'
DropCarryingNow.does = 'stonehearth:drop_carrying_now'
DropCarryingNow.args = {
   drop_always = {
      type = 'boolean',      -- whether to drop even if anim doesn't finish
      default = false,    -- false by default
   }
}
DropCarryingNow.version = 2
DropCarryingNow.priority = 1

function DropCarryingNow:start_thinking(ai, entity, args)
   ai.CURRENT.carrying = nil
   ai:set_think_output()
end

function DropCarryingNow:run(ai, entity, args)
   self._curr_carrying = radiant.entities.get_carrying(entity)
   if self._curr_carrying then
      local location = radiant.entities.get_world_grid_location(entity)

      -- xxx: make sure we can actually drop there, right?
      local pt = entity:get_component('mob'):get_location_in_front()
      radiant.entities.turn_to_face(entity, pt)

      ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })
      radiant.entities.drop_carrying_on_ground(entity, Point3(pt.x, pt.y, pt.z))
   end
end

function DropCarryingNow:stop(ai, entity, args)
   -- We may get here without having dropped anything because the effect
   -- was interrupted. If we should drop unconditionally, then do
   local curr_carrying = radiant.entities.get_carrying(entity)
   if args.drop_always and
      curr_carrying and
      self._curr_carrying and
      self._curr_carrying:get_id() == curr_carrying:get_id() then
 
      local location = radiant.entities.get_world_grid_location(entity)
      radiant.entities.drop_carrying_on_ground(entity, location)
   end

end

return DropCarryingNow
