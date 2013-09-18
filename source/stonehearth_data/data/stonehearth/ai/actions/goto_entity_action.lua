local GotoEntityAction = class()

GotoEntityAction.name = 'stonehearth.actions.goto_entity'
GotoEntityAction.does = 'stonehearth.activities.goto_entity'
GotoEntityAction.priority = 1
--TODO we need a scale to  describe relative importance

function GotoEntityAction:run(ai, entity, target)
   radiant.check.is_entity(target)
   
   local path = nil
   local solved = function(solution)
      path = solution
   end
   
   local pathfinder = _radiant.sim.create_path_finder('goto entity action', entity, solved, nil)
   pathfinder:add_destination(target)
   
   -- xxx: if the item moves, we want to bail immediately, not run to its new
   -- location!
   ai:wait_until(function()
      return path ~= nil
   end)
   ai:execute('stonehearth.activities.follow_path', path)
end

return GotoEntityAction
