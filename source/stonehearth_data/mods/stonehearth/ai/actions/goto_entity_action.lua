local GotoEntityAction = class()

GotoEntityAction.name = 'stonehearth.actions.goto_entity'
GotoEntityAction.does = 'stonehearth.goto_entity'
GotoEntityAction.priority = 1
--TODO we need a scale to  describe relative importance

function GotoEntityAction:run(ai, entity, target)
   radiant.check.is_entity(target)
   
   local pathfinder = _radiant.sim.create_path_finder('goto entity action')
                         :set_source(entity)
                         :add_destination(target)

   local path = ai:wait_for_path_finder(pathfinder)

   ai:execute('stonehearth.follow_path', path)
end

return GotoEntityAction
