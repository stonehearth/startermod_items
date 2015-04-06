local Point3 = _radiant.csg.Point3

local FindPointBeyondVisibleRegion = class()

FindPointBeyondVisibleRegion.name = 'find point beyond visible region'
FindPointBeyondVisibleRegion.does = 'stonehearth:find_point_beyond_visible_region'
FindPointBeyondVisibleRegion.args = {}
FindPointBeyondVisibleRegion.think_output = {
   location = Point3
}
FindPointBeyondVisibleRegion.version = 2
FindPointBeyondVisibleRegion.priority = 1

function FindPointBeyondVisibleRegion:start_thinking(ai, entity, args)
   self._thinking_started = true
   stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity_astar(
      entity, entity, 20, 5, 500,
      function (found_point)
         --Success function
         if self._thinking_started then
            ai:set_think_output({location = found_point})
         end
      end, 
      function ()
         --Fail function, pick some location near self
         if self._thinking_started then
            local location = radiant.terrain.find_placement_point(radiant.entities.get_world_grid_location(entity), 5, 30)
            ai:set_think_output({location = location})
         end
      end)
end

function FindPointBeyondVisibleRegion:stop_thinking(ai, entity, args)
   self._thinking_started = false
end

return FindPointBeyondVisibleRegion