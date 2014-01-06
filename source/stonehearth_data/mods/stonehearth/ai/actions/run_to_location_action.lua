local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local RunToLocation = class()

RunToLocation.name = 'run to location'
RunToLocation.does = 'stonehearth:goto_location'
RunToLocation.args = {
   Point3,     -- location to go to
   'string'    -- default effect to use when travelling
}
RunToLocation.version = 2
RunToLocation.priority = 1

function RunToLocation:start_background_processing(ai, entity, location, effect_name)
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
   radiant.terrain.place_entity(self._dest_entity, location)
   ai:complete_background_processing(self._dest_entity, effect_name)
end

function RunToLocation:stop_background_processing(ai, entity)
   if self._dest_entity then
      radiant.entities.destroy(self._dest_entity)
      self._dest_entity = nil
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(RunToLocation)
         :execute('stonehearth:goto_entity', ai.PREV[1], ai.ARGS[2])

