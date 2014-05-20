local Entity = _radiant.om.Entity
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local WalkAroundEntity = class()
WalkAroundEntity.name = 'walk around entity'
WalkAroundEntity.does = 'stonehearth:walk_around_entity'
WalkAroundEntity.args = {
   target = Entity,
   distance = 'number',
}
WalkAroundEntity.version = 2
WalkAroundEntity.priority = 1

function WalkAroundEntity:run(ai, entity, args)
   local waypoints = self:_get_patrol_points(args)
   local index = self:_index_of_closest_point(entity, waypoints)
   self:_rotate_array(waypoints, index-1)

   ai:execute('stonehearth:go_toward_location', { destination = waypoints[1] })
   ai:execute('stonehearth:go_toward_location', { destination = waypoints[2] })
   ai:execute('stonehearth:go_toward_location', { destination = waypoints[3] })
   ai:execute('stonehearth:go_toward_location', { destination = waypoints[4] })

   -- close the loop at the starting location
   ai:execute('stonehearth:go_toward_location', { destination = waypoints[1] })
end

function WalkAroundEntity:_rotate_array(array, times)
   for i = 1, times do
      local value = table.remove(array, 1)
      table.insert(array, value)
   end
end

function WalkAroundEntity:_index_of_closest_point(entity, points)
   local location = radiant.entities.get_world_grid_location(entity)
   local closest_index, closest_distance, distance

   for index, point in ipairs(points) do
      distance = location:distance_to(point)
      if not closest_distance or distance < closest_distance then
         closest_index, closest_distance = index, distance
      end
   end

   return closest_index
end

function WalkAroundEntity:_get_patrol_points(args)
   local location = radiant.entities.get_world_grid_location(args.target)
   local collision_component = args.target:get_component('region_collision_shape')

   if collision_component ~= nil then
      local entity_region = collision_component:get_region():get()
      local bounds = entity_region:get_bounds():translated(location)
      local inscribed_bounds = self:_get_inscribed_bounds(bounds)
      local expanded_bounds = self:_expand_cube_xz(inscribed_bounds, args.distance)
      local clockwise = rng:get_int(0, 1) == 1
      local patrol_points = self:_get_projected_vertices(expanded_bounds, clockwise)
      return patrol_points
   else
      return { location }
   end
end

-- terrain aligned entities have their position shifted by (-0.5, 0, -0.5)
-- this returns the grid aligned points enclosed by the bounds when the entity is placed on the terrain
function WalkAroundEntity:_get_inscribed_bounds(bounds)
   local min = bounds.min
   local max = bounds.max
   return Cube3(
      min,
      Point3(max.x-1, max.y, max.z-1)
   )
end

-- make cube larger in the x and z dimensions
function WalkAroundEntity:_expand_cube_xz(cube, distance)
   local min = cube.min
   local max = cube.max
   return Cube3(
      Point3(min.x - distance, min.y, min.z - distance),
      Point3(max.x + distance, max.y, max.z + distance)
   )
end

function WalkAroundEntity:_get_projected_vertices(cube, clockwise)
   local min = cube.min
   local max = cube.max
   local y = min.y
   local verticies = {
      -- in counterclockwise order
      Point3(min.x, y, min.z),
      Point3(min.x, y, max.z),
      Point3(max.x, y, max.z),
      Point3(max.x, y, min.z)
   }

   if clockwise then
      -- make clockwise
      verticies[2], verticies[4] = verticies[4], verticies[2]
   end
   
   return verticies
end

return WalkAroundEntity
