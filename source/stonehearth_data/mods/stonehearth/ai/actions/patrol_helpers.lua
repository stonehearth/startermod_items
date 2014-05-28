local PatrolHelpers = require 'ai.actions.optimized_pathfinder'
local Point3 = _radiant.csg.Point3

local PatrolHelpers = class()

-- find the "best" order to traverse the waypoints
function PatrolHelpers.order_waypoints(entity, start_location, waypoints)
   -- find the waypoint closest to the entity
   local index = PatrolHelpers._index_of_closest_point(start_location, waypoints)

   -- rotate the route so that it starts with the closest point
   PatrolHelpers._rotate_array(waypoints, index-1)

   -- close the loop at the starting location
   table.insert(waypoints, waypoints[1])
end

function PatrolHelpers.prune_non_standable_points(entity, points)
   -- remove in reverse so we don't have to worry about iteration order
   for i = #points, 1, -1 do
      if not radiant.terrain.is_standable(entity, points[i]) then
         table.remove(points, i)
      end
   end
end

function PatrolHelpers._rotate_array(array, times)
   local temp = {}

   -- the new index of the last element of the old array
   local temp_index = #array - times

   -- save the values that will be rotated to the end of the array
   for i = 1, times do
      temp[i] = array[i]
   end   

   -- advance the array position by times
   for i = 1, temp_index do
      array[i] = array[times+i]
   end

   -- copy the rotated values to their new location
   for i = 1, times do
      array[temp_index+i] = temp[i]
   end
end

function PatrolHelpers._index_of_closest_point(location, points)
   local closest_index, closest_distance, distance

   for index, point in ipairs(points) do
      distance = location:distance_to(point)
      if not closest_distance or distance < closest_distance then
         closest_index, closest_distance = index, distance
      end
   end

   return closest_index
end

return PatrolHelpers
