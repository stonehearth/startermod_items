local log = radiant.log.create_logger('follow_path')

local FollowPath = class()

function FollowPath:__init(entity, speed, path)
   assert(entity:add_component('mob'):get_parent() == radiant.entities.get_root_entity(),
          'can only move entities that are children of the root entity')

   self._entity = entity
   self._speed = speed
   self._stop_distance = 0
   self._mob = self._entity:add_component('mob')

   local full_path = self:_get_full_path(path)
   self:_initialize_path(full_path)
end

function FollowPath:set_speed(speed)
   self._speed = speed
   return self
end

function FollowPath:set_stop_distance(stop_distance)
   self._stop_distance = stop_distance
   return self
end

function FollowPath:set_arrived_cb(arrived_cb)
   self._arrived_cb = arrived_cb
   return self
end

function FollowPath:set_aborted_cb(aborted_cb)
   self._aborted_cb = aborted_cb
   return self
end

function FollowPath:start()
   if not self._path then
      self:_do_aborted()
      return self
   end

   self:_check_ignore_stop_distance()
   if self:_arrived() then
      self:_do_arrived()
   else
      self._gameloop_listener = radiant.events.listen(radiant, 'stonehearth:gameloop', self, self._move_one_gameloop)
   end
   return self
end

function FollowPath:stop()
   if self._gameloop_listener then
      self._gameloop_listener:destroy()
      self._gameloop_listener = nil
   end
   return self
end

function FollowPath:_initialize_path(path)
   self._path = path

   if not self._path then
      return
   end

   -- path:get_points and path:get_pruned_points return lua base 1 arrays
   self._points = self._path:get_pruned_points()
   self._num_points = #self._points
   self._last_index = self._num_points
   self._poi = self._path:get_destination_point_of_interest()
   self._finish_point = self._path:get_finish_point()
   self._destination_y = self._finish_point.y
   assert(self._finish_point == self._points[self._last_index])

   self._contiguous_points = self._path:get_points()
   self._contiguous_index = 1
   assert(self:_is_contiguous_path(self._contiguous_points))

   self._pursuing_index = self:_calculate_start_index()
   self._stop_index = self:_calculate_stop_index()

   log:debug('%s following path: start_index: %d, stop_index: %d, num_points: %d',
             self._entity, self._pursuing_index, self._stop_index, self._num_points)
end

function FollowPath:_do_arrived()
   self:_trigger_arrived_cb()
   self:stop()
end

function FollowPath:_do_aborted()
   self:_trigger_aborted_cb()
   self:stop()
end

function FollowPath:_arrived()
   if self._pursuing_index < self._stop_index then
      return false
   end

   if self._pursuing_index > self._stop_index then
      return true
   end

   -- Don't travel all the way to stop_index if not necessary.
   -- This becomes important if we prune points out of the path so they they are not adjacent.
   -- If stop_distance == 0 then make sure we finish pursuing all points on the path.
   -- This is important when the entity and poi are at the same location and we're trying
   -- to move to a point in the adjacent region (e.g. to pick up the item).
   assert(self._pursuing_index == self._stop_index)

   local location = self._mob:get_location()

   -- only stop early if the current location is at the same elevation as the destination
   -- and the distance to the poi is less than the stop_distance
   if self._stop_distance > 0 and location.y == self._destination_y then
      local poi_distance = location:distance_to(self._poi)
      return poi_distance <= self._stop_distance
   else
      local goal = self._points[self._stop_index]
      return location == goal
   end
end

-- If stop_distance is non-zero, we want to be stop_distance away from the poi.
-- If we're too close and the finish point is further, just go to the finish point.
function FollowPath:_check_ignore_stop_distance()
   local location = self._mob:get_location()
   local location_to_poi_distance = location:distance_to(self._poi)

   if location_to_poi_distance < self._stop_distance then
      local finish_point_to_poi_distance = self._finish_point:distance_to(self._poi)

      if finish_point_to_poi_distance > location_to_poi_distance then
         log:info('%s Start location closer than stop_distance from the poi. Ignoring stop_distance and going to the finish point.', self._entity)
         self._stop_distance = 0
         self._stop_index = self._last_index
      end
   end
end

function FollowPath:_move_one_gameloop()
   local move_distance = self._speed * _radiant.sim.get_base_walk_speed()

   assert(self._mob:get_location() == self._mob:get_world_location(), 'entity must be a child of the root entity')

   while true do
      if self:_arrived() then
         self:_do_arrived()
         break
      end

      if move_distance <= 0 then
         -- ran as far as we could this gameloop
         break
      end

      local current_location = self._mob:get_location()
      local new_location
      local goal = self._points[self._pursuing_index]
      local goal_vector = goal - current_location
      local goal_distance = goal_vector:length()
      local speed_scale = radiant.terrain.get_movement_speed_at(current_location)
      local real_move_distance = move_distance * speed_scale

      if goal_distance <= real_move_distance or goal_distance == 0 then
         -- enough movement distance to reach the next point
         new_location = goal
         move_distance = move_distance - (goal_distance / speed_scale)
         self._pursuing_index = self._pursuing_index + 1
      else
         -- movement distance stops short of the next point
         new_location = current_location + goal_vector * (real_move_distance / goal_distance)
         move_distance = 0
      end

      -- verify that the move is still valid because the the terrain may have changed from when we found the path
      if not self:_is_valid_move(current_location, new_location) then
         log:info('%s Follow path aborting because terrain/nav_grid has changed and move is no longer valid', self._entity)
         self:_do_aborted()
         break
      end

      self._mob:move_to(new_location)
      self._mob:turn_to_face_point(goal)
   end
end

-- 1) Note that the entity's current grid location may not be on a voxel in the contiguous set
--    a) because diagonal movement usually crosses non-diagonal adjacent cells
--    b) because the mob does not move from voxel center to voxel center
-- 2) Do not assume that the mob's grid location is standable (e.g. it maybe dropping 2 voxels down).
-- 3) Do not terminate movement until we've finished seeking the current contigous point.
--    i.e. we don't want to stop halfway when climbing up or down an adjacent voxel since we'll clip into the terrain.
--
-- So, we define an entity to have reached a point when we transition from moving towards the point
-- to moving away from the point. At this time, we validate the next contiguous move.
function FollowPath:_is_valid_move(from, to)
   local current_point = self._contiguous_points[self._contiguous_index]

   -- iterate until we're moving towards the next contiguous point
   -- this better handles cases where the entity's start location does not match the path start location
   -- or when the entity is bumped off the path
   while self:_moving_away_from_point(from, to, current_point) do
      -- validate the move from the current point to the next point
      self._contiguous_index = self._contiguous_index + 1
      local next_point = self._contiguous_points[self._contiguous_index]

      local valid_move = _radiant.sim.is_valid_move(self._entity, false, current_point, next_point)
      if not valid_move then
         return false
      end
      
      current_point = next_point
   end

   return true
end

function FollowPath:_moving_away_from_point(from, to, point)
   local move_vector = to - from
   local location_vector = point - from

   -- we're moving away from the point if the dot product is negative
   local moving_towards_point = move_vector:dot(location_vector) < 0
   return moving_towards_point
end

function FollowPath:_calculate_start_index()
   local start_location = radiant.entities.get_world_grid_location(self._entity)

   if self._num_points > 1 and self._points[1] == start_location then
      -- skip the first point so that we don't seek the center of the current voxel before starting on the path
      return 2
   else
      return 1
   end
end

function FollowPath:_calculate_stop_index()
   local previous_point = self._poi
   local current_point
   local index = self._last_index
   local distance = 0

   if self._stop_distance == 0 then
      return self._last_index
   end

   for i = self._last_index, 1, -1 do
      current_point = self._points[i]
      if current_point.y ~= self._destination_y then
         -- don't stop early if there is an elevation change between here and the destination
         return self._last_index
      end

      distance = distance + current_point:distance_to(previous_point)
      if distance > self._stop_distance then
         -- return the last index where the distance was closer than the stopDistance
         break
      end
      index = i
      previous_point = current_point
   end

   -- if the entire travelled path is still less than stop_distance, we don't need to move at all
   if index == 1 then
      local start_location = radiant.entities.get_world_location(self._entity)
      if startLocation:distance_to(self._poi) <= self._stop_distance then
         index = 0; -- yes, the index before the first point
      end
   end

   return index;
end

function FollowPath:_trigger_arrived_cb()
   if self._arrived_cb then
      self._arrived_cb()
   end
end

function FollowPath:_trigger_aborted_cb()
   if self._aborted_cb then
      self._aborted_cb()
   end
end

function FollowPath:_is_contiguous_path(points)
   for i=2, #points do
      local previous = points[i-1]
      local current = points[i]
      
      if previous == current then
         return false
      end
      
      local delta = current - previous
      delta.y = 0 -- consider xz movement only
      
      local distance_squared = delta:distance_squared()
      if distance_squared > 2 then
         return false
      end
   end

   return true
end

function FollowPath:_get_full_path(path)
   local location = self._mob:get_world_grid_location()
   local start_point = path:get_start_point()

   if location == start_point then
      return path
   end

   -- TODO: consider aborting if the distance to the start_point is too far
   log:info('%s not at start point %s. finding intercept path...', self._entity, start_point)

   local original_points = path:get_points()
   local intercept_point, intercept_index = self:_find_closest_point_on_path(original_points, location)

   -- find a direct path to the closest point on the main path
   local intercept_path = self:_find_direct_path(location, intercept_point)

   if not intercept_path then
      -- blocked? then try to find a path to the original starting location
      -- this is usually open since we just recently moved from there to here
      intercept_point = start_point
      intercept_index = 1
      intercept_path = self:_find_direct_path(location, intercept_point)
   end

   if intercept_path then
      log:info('%s adding intercept path from %s to %s', self._entity, location, intercept_point)

      local finish_index = #original_points
      local remainder_path = _radiant.sim.create_path_subset(path, intercept_index, finish_index)
      local full_path =  _radiant.sim.combine_paths({ intercept_path, remainder_path })
      return full_path
   else
      log:warning('%s unable to reach main path', self._entity)
      -- we could try harder, but best to just abort and defer back to a*
      return nil
   end
end

function FollowPath:_find_closest_point_on_path(path_points, from)
   local closest_point, closest_index, closest_distance

   for index, point in ipairs(path_points) do
      local distance = from:distance_to(point)

      if closest_distance and distance > closest_distance then
         -- exit once we find start getting farther away from the path
         -- note that this might miss an even closer point that occurs later, but is good enough for our purposes
         break
      end

      closest_point = point
      closest_index = index
      closest_distance = distance
   end

   return closest_point, closest_index
end

function FollowPath:_find_direct_path(from, to)
   local direct_path_finder = _radiant.sim.create_direct_path_finder(self._entity)
                              :set_start_location(from)
                              :set_end_location(to)
                              :set_allow_incomplete_path(false)
                              :set_reversible_path(false)

   local path = direct_path_finder:get_path()
   return path
end

return FollowPath
