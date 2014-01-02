local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local GotoLocation = class()


GotoLocation.name = 'goto location'
GotoLocation.does = 'stonehearth:goto_location'
GotoLocation.priority = 1

function GotoLocation:run(ai, entity, dest, effect_name)
   -- generally speaking, going directly to a location is a strange
   -- thing to do.  why did we not path find to an entity?  why is
   -- this location special?
   
   -- Commentary: No it's not. It's useful when coding ambient behavior
   -- like wandering around, running away, etc. - tom, 9/23/2013

   -- Commenary: Yes it is. =) You don't really know if you can get
   -- to an arbitray location in the world.  For example, if you try to
   -- goto a location inside a box, the pathfinder will just bail with
   -- no solution.  It makes more sense to have a vector based controller
   -- to implement ambient behavior. - tony, 10/01/2013

   -- anyway, the pathfinder can only find paths between two entities,
   -- so go ahead and make a new one.
   
   self._dest_entity = radiant.entities.create_entity()
   self._dest_entity:set_debug_text('goto location proxy entity')
   radiant.terrain.place_entity(self._dest_entity, dest)

   local pf = radiant.pathfinder.create_path_finder(entity, 'goto_location')
                  :add_destination(self._dest_entity)

   local path = ai:wait_for_path_finder(pf)

   ai:execute('stonehearth:follow_path', path, effect_name)
end

function GotoLocation:stop()
   if self._dest_entity then
      radiant.entities.destroy_entity(self._dest_entity)
      self._dest_entity = nil
   end
end

return GotoLocation
