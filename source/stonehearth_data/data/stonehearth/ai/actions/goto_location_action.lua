local GotoLocationAction = class()

GotoLocationAction.name = 'stonehearth.actions.goto_location'
GotoLocationAction.does = 'stonehearth.activities.goto_location'
GotoLocationAction.priority = 1

function GotoLocationAction:run(ai, entity, dest)

   local locations = PointList()
   locations:insert(dest)

   local dst = EntityDestination(entity, locations) -- wrong...!

   local pf = native:create_path_finder('goto_location', entity)
   pf:add_destination(dst)

   local path = nil
   ai:wait_until(function()
         path = pf:get_solution()
         return path ~= nil
      end)
   ai:execute('stonehearth.activities.follow_path', path)
end

return GotoLocationAction