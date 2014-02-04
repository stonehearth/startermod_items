local Point3 = _radiant.csg.Point3
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
   if radiant.entities.get_carrying(entity) then
      local location = radiant.entities.get_world_grid_location(entity)
      ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })
      radiant.entities.drop_carrying_on_ground(entity, location)
   end
end

return DropCarryingNow
