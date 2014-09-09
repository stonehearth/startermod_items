local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

PatrollableObject = class()

function PatrollableObject:initialize(object)
   assert(self._sv)

   self._sv.object = object
   self._sv.object_id = object:get_id()
   self._sv.last_patrol_time = radiant.gamestate.now()
   self.__saved_variables:mark_changed()
end

function PatrollableObject:get_last_patrol_time()
   return self._sv.last_patrol_time
end

function PatrollableObject:mark_visited()
   self._sv.last_patrol_time = radiant.gamestate.now()
   self.__saved_variables:mark_changed()
end

function PatrollableObject:get_id()
   return self._sv.object_id
end

-- this API will need to change when we have non-object patrol routes
function PatrollableObject:get_object()
   return self._sv.object
end

-- this API will need to change when we have non-object patrol routes
function PatrollableObject:get_centroid()
   local object = self:get_object()
   local location = radiant.entities.get_world_location(object)
   local collision_component = object:get_component('region_collision_shape')
   local centroid

   if collision_component ~= nil then
      local object_region = collision_component:get_region():get()
      centroid = _radiant.csg.get_region_centroid(object_region) + location
      centroid.y = location.y   -- project to the height plane of the origin
   else
      centroid = location
   end

   return centroid
end

function PatrollableObject:get_waypoints(distance, entity)
   local object = self:get_object()
   local location = radiant.entities.get_world_grid_location(object)
   local collision_component = object:get_component('region_collision_shape')

   if collision_component ~= nil then
      local object_region = collision_component:get_region():get()
      local bounds = object_region:get_bounds():to_int():translated(location)
      local clockwise = rng:get_int(0, 1) == 1
      local waypoints = self:_get_waypoints(bounds, distance, clockwise, entity)
      return waypoints
   else
      -- TODO: decide what waypoints to use in this case
      return { location }
   end
end

function PatrollableObject:acquire_lease(entity)
   local object = self:get_object()
   local acquired = radiant.entities.acquire_lease(object, 'stonehearth:patrol_lease', entity, true)
   return acquired
end

function PatrollableObject:can_acquire_lease(entity)
   local object = self:get_object()
   local can_acquire = radiant.entities.can_acquire_lease(object, 'stonehearth:patrol_lease', entity)
   return can_acquire
end

function PatrollableObject:release_lease(entity)
   local object = self:get_object()
   local released = radiant.entities.release_lease(object, 'stonehearth:patrol_lease', entity)
   return released
end

function PatrollableObject:_get_waypoints(bounds, distance, clockwise, entity)
   local min = bounds.min
   local max = bounds.max
   local start_point = Point3(0, min.y, 0)
   local desired_point, resolved_point
   local waypoints = {}

   start_point.x, start_point.z = min.x, min.z
   desired_point = start_point + Point3(-distance, 0, -distance)
   resolved_point = radiant.terrain.get_direct_path_end_point(start_point, desired_point, entity)
   table.insert(waypoints, resolved_point)

   start_point.x, start_point.z = max.x-1, min.z   -- -1 to convert from the bounds to the point
   desired_point = start_point + Point3(distance, 0, -distance)
   resolved_point = radiant.terrain.get_direct_path_end_point(start_point, desired_point, entity)
   table.insert(waypoints, resolved_point)

   start_point.x, start_point.z = max.x-1, max.z-1
   desired_point = start_point + Point3(distance, 0, distance)
   resolved_point = radiant.terrain.get_direct_path_end_point(start_point, desired_point, entity)
   table.insert(waypoints, resolved_point)

   start_point.x, start_point.z = min.x, max.z-1
   desired_point = start_point + Point3(-distance, 0, distance)
   resolved_point = radiant.terrain.get_direct_path_end_point(start_point, desired_point, entity)
   table.insert(waypoints, resolved_point)

   if not clockwise then
      waypoints[2], waypoints[4] = waypoints[4], waypoints[2]
   end

   return waypoints
end

return PatrollableObject
