local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('town_defense')

TownDefense = class()

-- Conventions adopted in this class:
--   entity: a unit (like a footman)
--   object: everything else, typically something that can be patrolled

-- helper classes
PatrolObject = class()
PatrolAssignment = class()

function PatrolObject:__init(object)
   self.object = object
   self.last_patrol_time = radiant.gamestate.now()
end

function PatrolObject:get_centroid()
   local location = radiant.entities.get_world_location(self.object)
   local collision_component = self.object:get_component('region_collision_shape')
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

function PatrolObject:mark_visited()
   self.last_patrol_time = radiant.gamestate.now()
end

function PatrolAssignment:__init(points, patrol_object)
   self.patrol_object = patrol_object
   self.points = points
   self.current_index = 1
end

function PatrolAssignment:get_current_point()
   return self.points[self.current_index]
end

function PatrolAssignment:on_last_point()
   return self.current_index >= #self.points
end

function PatrolAssignment:advance_to_next_point()
   self.current_index = self.current_index + 1
end

function TownDefense:initialize()
   -- push_object_state is called for us by trace_world_entities
   -- hold on to the trace so we have a refcount for it
   self._world_objects_trace = radiant.terrain.trace_world_entities('town defense service', 
      function (id, object)
         self:_on_object_added(object)
      end,
      function (id)
         self:_on_object_removed(id)
      end
   )

   self._player_id_traces = {}
   self._patrol_objects = {}
   self._patrol_assignments = {}
end

function TownDefense:destroy()
end

function TownDefense:get_patrol_point(entity)
   local entity_id = entity:get_id()
   local patrol_assignment = self._patrol_assignments[entity_id]

   if patrol_assignment and not patrol_assignment:on_last_point() then
      patrol_assignment:advance_to_next_point()
      return patrol_assignment:get_current_point()
   end

   local patrol_object = self:_get_object_to_patrol(entity)

   if patrol_object then
      local waypoints = self:_create_patrol_route(entity, patrol_object.object)
      local patrol_assignment = PatrolAssignment(waypoints, patrol_object)
      local acquired_lease = self:_acquire_lease('stonehearth:patrol_lease', patrol_object.object, entity)

      if acquired_lease then
         self._patrol_assignments[entity_id] = patrol_assignment
         return patrol_assignment:get_current_point()
      end
   end

   self._patrol_assignments[entity_id] = nil
   return nil
end

-- CHECKCHECK - automatically release leases after a certian time
function TownDefense:mark_patrol_complete(entity)
   local entity_id = entity:get_id()
   local patrol_assignment = self._patrol_assignments[entity_id]
   if patrol_assignment then
      local patrol_object = patrol_assignment.patrol_object
      patrol_object:mark_visited()
      self:_release_lease('stonehearth:patrol_lease', patrol_object.object)
   else
      log:error('patrol assignment not found for %s', entity)
   end
end

function TownDefense:_get_object_to_patrol(entity)
   local location = radiant.entities.get_world_location(entity)
   local unleased_objects = self:_get_unleased_objects(entity)
   local ordered_objects = {}

   if next(unleased_objects) == nil then
      return nil
   end

   for object_id, patrol_object in pairs(unleased_objects) do
      local distance = location:distance_to(patrol_object:get_centroid())
      if distance < 1 then
         distance = 1
      end
      local time = radiant.gamestate.now() - patrol_object.last_patrol_time
      local score = time / distance
      local sort_info = { score = score, patrol_object = patrol_object }
      table.insert(ordered_objects, sort_info)
   end

   table.sort(ordered_objects,
      function (a, b)
         return a.score > b.score
      end
   )

   local patrol_object = ordered_objects[1].patrol_object
   return patrol_object
end

function TownDefense:_create_patrol_route(entity, object)
   local patrol_margin = 2
   local waypoints = self:_get_waypoints_around_object(object, patrol_margin)
   local index = self:_index_of_closest_point(entity, waypoints)

   -- rotate the route so that it starts with the closest point
   self:_rotate_array(waypoints, index-1)

   -- close the loop at the starting location
   table.insert(waypoints, waypoints[1])

   return waypoints
end

function TownDefense:_on_object_added(object)
   if self:_is_patrollable(object) then
      self:_add_to_patrol_list(object)
      -- trace all objects that are patrollable in case their ownership changes
      self:_add_player_id_trace(object)
   end
end

function TownDefense:_add_player_id_trace(object)
   local object_id = object:get_id()
   local player_id_trace = object:add_component('unit_info'):trace_player_id('town defense')
      :on_changed(
         function ()
            self:_on_player_id_changed(object)
         end
      )

   -- keep a refcount for the trace so it stays around
   self._player_id_traces[object_id] = player_id_trace
end

function TownDefense:_remove_player_id_trace(object_id)
   self._player_id_traces[object_id] = nil
end

function TownDefense:_on_player_id_changed(object)
   self:_remove_from_patrol_list(object:get_id())
   self:_add_to_patrol_list(object)
end

function TownDefense:_on_object_removed(object_id)
   self:_remove_from_patrol_list(object_id)
   self:_remove_player_id_trace(object_id)
end

function TownDefense:_is_patrollable(object)
   if object == nil or not object:is_valid() then
      return false
   end

   local entity_data = radiant.entities.get_entity_data(object, 'stonehearth:town_defense')
   return entity_data and entity_data.auto_patrol == true
end

function TownDefense:_add_to_patrol_list(object)
   local object_id = object:get_id()
   local player_id = radiant.entities.get_player_id(object)

   if player_id then
      local player_patrol_objects = self:_get_patrol_objects(player_id)
      if not player_patrol_objects[object_id] then
         player_patrol_objects[object_id] = PatrolObject(object)
      end
   end
end

function TownDefense:_remove_from_patrol_list(object_id)
   -- CHECKCHECK - eww, do this better
   for player_id, player_patrol_objects in pairs(self._patrol_objects) do
      if player_patrol_objects[object_id] then
         -- CHECKCHECK - recalculate existing routes
         player_patrol_objects[object_id] = nil
         return
      end
   end
end

function TownDefense:_get_patrol_objects(player_id)
   local player_patrol_objects = self._patrol_objects[player_id]
   if not player_patrol_objects then
      player_patrol_objects = {}
      self._patrol_objects[player_id] = player_patrol_objects
   end
   return player_patrol_objects
end

function TownDefense:_get_unleased_objects(entity)
   local player_id = radiant.entities.get_player_id(entity)
   local player_patrol_objects = self:_get_patrol_objects(player_id)
   local unleased_objects = {}

   for object_id, patrol_object in pairs(player_patrol_objects) do
      local lease_component = patrol_object.object:add_component('stonehearth:lease')
      if lease_component:can_acquire('stonehearth:patrol_lease', entity) then
         unleased_objects[object_id] = patrol_object
      end
   end

   return unleased_objects
end

function TownDefense:_rotate_array(array, times)
   for i = 1, times do
      local value = table.remove(array, 1)
      table.insert(array, value)
   end
end

function TownDefense:_index_of_closest_point(entity, points)
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

function TownDefense:_get_waypoints_around_object(object, distance)
   local location = radiant.entities.get_world_grid_location(object)
   local collision_component = object:get_component('region_collision_shape')

   if collision_component ~= nil then
      local object_region = collision_component:get_region():get()
      local bounds = object_region:get_bounds():translated(location)
      local inscribed_bounds = self:_get_inscribed_bounds(bounds)
      local expanded_bounds = self:_expand_cube_xz(inscribed_bounds, distance)
      local clockwise = rng:get_int(0, 1) == 1
      local patrol_points = self:_get_projected_vertices(expanded_bounds, clockwise)
      return patrol_points
   else
      return { location }
   end
end

-- terrain aligned objects have their position shifted by (-0.5, 0, -0.5)
-- this returns the grid aligned points enclosed by the bounds when the object is placed on the terrain
function TownDefense:_get_inscribed_bounds(bounds)
   local min = bounds.min
   local max = bounds.max
   return Cube3(
      min,
      Point3(max.x-1, max.y, max.z-1)
   )
end

-- make cube larger in the x and z dimensions
function TownDefense:_expand_cube_xz(cube, distance)
   local min = cube.min
   local max = cube.max
   return Cube3(
      Point3(min.x - distance, min.y, min.z - distance),
      Point3(max.x + distance, max.y, max.z + distance)
   )
end

function TownDefense:_get_projected_vertices(cube, clockwise)
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

function TownDefense:_acquire_lease(lease_name, leased_object, lease_holder)
   local lease_component = leased_object:add_component('stonehearth:lease')
   local acquired = lease_component:acquire(lease_name, lease_holder, { persistent = true })
   return acquired
end

function TownDefense:_release_lease(lease_name, leased_object)
   local lease_component = leased_object:add_component('stonehearth:lease')
   local result = lease_component:release(lease_name, leased_object)
   return result
end

return TownDefense
