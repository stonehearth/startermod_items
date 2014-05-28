local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

PatrollableObject = class()

function PatrollableObject:__init()
end

function PatrollableObject:initialize(object)
   self._object = object

   self.__saved_variables = radiant.create_datastore()
   self._sv = self.__saved_variables:get_data()

   self._sv.object_id = object:get_id()
   self._sv.last_patrol_time = radiant.gamestate.now()
end

function PatrollableObject:restore(datastore)
   self.__saved_variables = datastore
   self._sv = self.__saved_variables:get_data()
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
   local object = self._object

   if not object then
      object = radiant.entities.get_entity(self._sv.object_id)
      self._object = object
   end

   return object
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

function PatrollableObject:get_waypoints(distance)
   local object = self:get_object()
   local location = radiant.entities.get_world_grid_location(object)
   local collision_component = object:get_component('region_collision_shape')

   if collision_component ~= nil then
      local object_region = collision_component:get_region():get()
      local bounds = object_region:get_bounds():translated(location)
      local inscribed_bounds = self:_get_inscribed_bounds(bounds)
      local expanded_bounds = self:_expand_cube_xz(inscribed_bounds, distance)
      local clockwise = rng:get_int(0, 1) == 1
      local waypoints = self:_get_projected_vertices(expanded_bounds, clockwise)
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

-- terrain aligned objects have their position shifted by (-0.5, 0, -0.5)
-- this returns the grid aligned points enclosed by the bounds when the object is placed on the terrain
function PatrollableObject:_get_inscribed_bounds(bounds)
   local min = bounds.min
   local max = bounds.max
   return Cube3(
      min,
      Point3(max.x-1, max.y, max.z-1)
   )
end

-- make cube larger in the x and z dimensions
function PatrollableObject:_expand_cube_xz(cube, distance)
   local min = cube.min
   local max = cube.max
   return Cube3(
      Point3(min.x - distance, min.y, min.z - distance),
      Point3(max.x + distance, max.y, max.z + distance)
   )
end

function PatrollableObject:_get_projected_vertices(cube, clockwise)
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

return PatrollableObject
