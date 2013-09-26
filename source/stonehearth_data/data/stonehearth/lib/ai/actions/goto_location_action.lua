local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local GotoLocationAction = class()


GotoLocationAction.name = 'stonehearth.actions.goto_location'
GotoLocationAction.does = 'stonehearth.activities.goto_location'
GotoLocationAction.priority = 1

function GotoLocationAction:run(ai, entity, dest)
   -- generally speaking, going directly to a location is a strange
   -- thing to do.  why did we not path find to an entity?  why is
   -- this location special?
   
   -- Commentary: No it's not. It's useful when coding ambient behavior
   -- like wandering around, running away, etc. - tom, 9/23/2013

   -- anyway, the pathfinder can only find paths between two entities,
   -- so go ahead and make a new one.  this is HORRIBLY INEFFICENT. =..(
   
   local bounds = Cube3(Point3(0, 0, 0), Point3(1, 1, 1))
   self._dest_entity = radiant.entities.create_entity()
   
   radiant.terrain.place_entity(self._dest_entity, dest)

   local path
   local solved = function(p)
      path = p
   end
   
   local pf = _radiant.sim.create_path_finder('goto_location', entity, solved, nil)
   pf:add_destination(self._dest_entity)
   ai:wait_until(function()
         return path ~= nil
      end)
   pf = nil
   ai:execute('stonehearth.activities.follow_path', path)
end

function GotoLocationAction:stop()
   if self._dest_entity then
      radiant.entities.destroy_entity(self._dest_entity)
   end
end

return GotoLocationAction