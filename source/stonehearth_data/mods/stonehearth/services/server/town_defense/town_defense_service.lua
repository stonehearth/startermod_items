local PatrollableObject = require 'services.server.town_defense.patrollable_object'
local PatrolRoute = require 'services.server.town_defense.patrol_route'
local log = radiant.log.create_logger('town_defense')

TownDefense = class()

-- Conventions adopted in this class:
--   entity: a unit (like a footman)
--   object: everything else, typically something that can be patrolled

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

   -- holds all the patrollable objects in the world organized by player_id
   self._patrollable_objects = {}

   -- map of entities to their current patrol routes
   self._patrol_assignments = {}

   -- traces for change of ownership of a patrollable object
   self._player_id_traces = {}

   -- maps patrollable objects to their owning player. used to provide O(1) destroy.
   self._object_to_player_map = {}
end

function TownDefense:destroy()
end

function TownDefense:get_patrol_point(entity)
   local entity_id = entity:get_id()
   local patrol_route = self._patrol_assignments[entity_id]

   -- do we have anything left to do on our current assignment?
   if patrol_route and not patrol_route:on_last_point() then
      -- if so, return the next waypoint
      patrol_route:advance_to_next_point()
      return patrol_route:get_current_point()
   end

   -- otherwise, create a new patrol route
   patrol_route = self:_get_patrol_route(entity)

   if patrol_route then
      self._patrol_assignments[entity_id] = patrol_route
      return patrol_route:get_current_point()
   else
      return nil
   end
end

-- CHECKCHECK - automatically release leases after a certain time
function TownDefense:mark_patrol_complete(entity)
   local entity_id = entity:get_id()
   local patrol_route = self._patrol_assignments[entity_id]

   if patrol_route then
      local patrollable_object = patrol_route.patrollable_object
      patrollable_object:mark_visited()
      self:_release_lease(patrollable_object.object)
      self:_trigger_patrol_point_available()
   else
      log:error('patrol assignment not found for %s', entity)
   end
end

function TownDefense:_get_patrol_route(entity)
   local patrol_route = nil

   while true do
      local patrollable_object = self:_get_object_to_patrol(entity)

      if not patrollable_object then
         -- nothing available, so just exit
         patrol_route = nil
         break
      end

      local waypoints = self:_create_patrol_route(entity, patrollable_object)

      if waypoints then
         patrol_route = PatrolRoute(waypoints, patrollable_object)
         local acquired_lease = self:_acquire_lease(patrollable_object.object, entity)

         if acquired_lease then
            break
         end

         -- lease should have already been checked in _get_object_to_patrol, we should never get here
         log:error('patrol_lease could not be acquired')
      else
         -- no patrol points are valid for this object
         -- mark it as visited and find the next best object
         patrollable_object:mark_visited()
         log:warning('could not create patrol route around %s', patrollable_object.object)
      end
   end

   return patrol_route
end

-- returns the patrollable object that is the best fit for this entity
function TownDefense:_get_object_to_patrol(entity)
   local location = radiant.entities.get_world_location(entity)
   local unleased_objects = self:_get_unleased_objects(entity)
   local ordered_objects = {}

   if next(unleased_objects) == nil then
      return nil
   end

   -- score each object based on the benefit/cost ratio of patrolling it
   for object_id, patrollable_object in pairs(unleased_objects) do
      local score = self:_calculate_patrol_score(location, patrollable_object)
      local sort_info = { score = score, patrollable_object = patrollable_object }
      table.insert(ordered_objects, sort_info)
   end

   -- order the objects by highest score
   table.sort(ordered_objects,
      function (a, b)
         return a.score > b.score
      end
   )

   -- return the object with the highest score
   local patrollable_object = ordered_objects[1].patrollable_object
   return patrollable_object
end

-- estimate the benefit/cost ratio of patrolling an object
-- cost is the distance to travel to the object
-- benefit is the amount of time since we last patrolled it
function TownDefense:_calculate_patrol_score(start_location, patrollable_object)
   local distance = start_location:distance_to(patrollable_object:get_centroid())
   -- add the minimum cost to patrol the perimeter of a 1x1 object at distance 2
   -- this keeps really close objects from getting an overinflated score because there is
   -- a nonzero cost to patrolling them 
   distance = distance + 14

   local time = radiant.gamestate.now() - patrollable_object.last_patrol_time
   -- if time is zero, then let distance be the tiebreaker
   if time < 1 then
      time = 1
   end
   
   local score = time / distance
   return score
end

function TownDefense:_create_patrol_route(entity, patrollable_object)
   local patrol_margin = 2
   local waypoints = patrollable_object:get_waypoints(patrol_margin)

   self:_prune_non_standable_points(entity, waypoints)

   if not next(waypoints) then
      return nil
   end

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

function TownDefense:_on_object_removed(object_id)
   self:_remove_from_patrol_list(object_id)
   self:_remove_player_id_trace(object_id)
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
      local player_patrollable_objects = self:_get_patrollable_objects(player_id)

      if not player_patrollable_objects[object_id] then
         player_patrollable_objects[object_id] = PatrollableObject(object)
         self._object_to_player_map[object_id] = player_id
         self:_trigger_patrol_point_available()
      end
   end
end

function TownDefense:_remove_from_patrol_list(object_id)
   local player_id = self._object_to_player_map[object_id]
   self._object_to_player_map[object_id] = nil

   if player_id then
      local player_patrollable_objects = self:_get_patrollable_objects(player_id)

      if player_patrollable_objects[object_id] then
         self:_remove_invalid_patrol_routes(object_id)
         player_patrollable_objects[object_id] = nil
      end
   end
end

function TownDefense:_remove_invalid_patrol_routes(object_id)
   for entity_id, patrol_route in pairs(self._patrol_assignments) do
      if patrol_route.patrollable_object.object_id == object_id then
         self._patrol_assignments[entity_id] = nil
      end
   end
end

function TownDefense:_get_patrollable_objects(player_id)
   local player_patrollable_objects = self._patrollable_objects[player_id]

   if not player_patrollable_objects then
      player_patrollable_objects = {}
      self._patrollable_objects[player_id] = player_patrollable_objects
   end

   return player_patrollable_objects
end

function TownDefense:_get_unleased_objects(entity)
   local player_id = radiant.entities.get_player_id(entity)
   local player_patrollable_objects = self:_get_patrollable_objects(player_id)
   local unleased_objects = {}

   for object_id, patrollable_object in pairs(player_patrollable_objects) do
      if self:_can_acquire_lease(entity) then
         unleased_objects[object_id] = patrollable_object
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

function TownDefense:_prune_non_standable_points(entity, points)
   -- remove in reverse so we don't have to worry about iteration order
   for i = #points, 1, -1 do
      if not radiant.terrain.can_stand_on(entity, points[i]) then
         table.remove(points, i)
      end
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

function TownDefense:_trigger_patrol_point_available()
   radiant.events.trigger_async(stonehearth.town_defense, 'stonehearth:patrol_point_available')
end

function TownDefense:_can_acquire_lease(leased_object, lease_holder)
   local lease_component = leased_object:add_component('stonehearth:lease')
   local can_acquire = lease_component:can_acquire('stonehearth:patrol_lease', lease_holder)
   return can_acquire
end

function TownDefense:_acquire_lease(leased_object, lease_holder)
   local lease_component = leased_object:add_component('stonehearth:lease')
   local acquired = lease_component:acquire('stonehearth:patrol_lease', lease_holder, { persistent = true })
   return acquired
end

function TownDefense:_release_lease(leased_object)
   local lease_component = leased_object:add_component('stonehearth:lease')
   local result = lease_component:release('stonehearth:patrol_lease', leased_object)
   return result
end

return TownDefense
