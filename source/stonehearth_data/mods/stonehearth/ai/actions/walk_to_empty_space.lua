local Point3 = _radiant.csg.Point3

local WalkToEmptySpace = class()

WalkToEmptySpace.name = 'walk to empty space'
WalkToEmptySpace.does = 'stonehearth:walk_to_empty_space'
WalkToEmptySpace.args = {
   radius = 'number',  --how far to look for location
   radius_min = {
      type = 'number', --min how far
      default = 0
   },
}
WalkToEmptySpace.version = 2
WalkToEmptySpace.priority = 1

--A lot like wander, but will only go to a location that is empty
--(so we can drop whatever we're carrying there, presumably)

function WalkToEmptySpace:run(ai, entity, args)
   local origin = radiant.entities.get_world_grid_location(entity)
   local target = radiant.terrain.find_placement_point(origin, args.radius_min, args.radius)
   ai:execute('stonehearth:goto_location', {location = target})
end

return WalkToEmptySpace