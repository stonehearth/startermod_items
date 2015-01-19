local SpawnRegionFinderService = class()
local rng = _radiant.csg.get_default_rng()
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('spawn_svc')

function SpawnRegionFinderService:initialize()
end


function _compute_and_check_points(point, entities, displacements)
   local results = { point }
   for i = 2,#entities do
      local p2 = radiant.terrain.get_standable_point(entities[i], point + displacements[i])

      local direct_path_finder = _radiant.sim.create_direct_path_finder(entities[i])
                                    :set_start_location(p2)
                                    :set_end_location(point)
      local path = direct_path_finder:get_path()
      if not path then
         return nil
      end
      table.insert(results, p2)
   end
   return results
end

function _center_of_hull(perimeter)
   -- Compute the center of the hull.
   local center = Point3(0, 0, 0)
   for _, p in pairs(perimeter) do
      center = center + p
   end

   return Point3(center.x / #perimeter, center.y / #perimeter, center.z / #perimeter)
end

function _generate_random_point_outside_perimeter(perimeter, distance)
   if #perimeter < 3 then
      return nil
   end
   -- Generate a random direction that will point in the direction of our generated point.
   local dir = radiant.math.random_xz_unit_vector()
   local normal = Point3(dir.z, 0, -dir.x)
   local hull_center = _center_of_hull(perimeter)
   local d = normal:dot(hull_center)
   for idx, _ in pairs(perimeter) do
      local p1 = perimeter[idx]
      local idx2 = (idx + 1) % (#perimeter + 1)
      if idx2 == 0 then idx2 = 1 end
      local p2 = perimeter[idx2]

      -- Look for the first pair of consecutive perimeter points that straddle the
      -- generated direction.
      if p1:dot(normal) > d and p2:dot(normal) <= d then
         -- Compute the intersection of the hull and that direction.
         local perimeter_line = p2 - p1
         local perimeter_normal = Point3(perimeter_line.z, 0, -perimeter_line.x)
         perimeter_normal:normalize()

         local hull_point = radiant.math.intersect_plane_with_line(hull_center, dir, p1, perimeter_normal)

         -- Inside a convex hull, we should always be able to intersect our line.
         assert(hull_point)

         -- Generate the point!
         return hull_point + (dir * distance)
      end
   end

   -- This should never happen.
   assert(false)
   return nil
end

function SpawnRegionFinderService:find_standable_points_outside_civ_perimeter(entities, displacements, distance)
   assert(displacements[1] == Point3(0, 0, 0))

   local p = self:find_point_outside_civ_perimeter_for_entity(entities[1], distance)

   if not p then
      return nil
   end

   return _compute_and_check_points(p, entities, displacements)
end


function SpawnRegionFinderService:find_standable_points_outside_civ_perimeter_astar(entities, destination, displacements, distance, max_attempts, max_steps, success_cb, fail_cb)
   assert(displacements[1] == Point3(0, 0, 0))
   local p = self:find_point_outside_civ_perimeter_for_entity_astar(entities[1], destination, distance, max_attempts, max_steps, function(p)
         local results = _compute_and_check_points(p, entities, displacements)
         if results then
            success_cb(results)
         else
            fail_cb()
         end
      end,
      function()
         fail_cb()
      end)
end


-- Find a point 'distance' units outside the perimeter of the explored area of the civilization 
-- to spawn an entity.
function SpawnRegionFinderService:find_point_outside_civ_perimeter_for_entity(entity, distance)

   -- Consult the convex hull of points that the civs have travelled.
   local player_perimeter = stonehearth.terrain:get_player_perimeter('civ')

   if #player_perimeter < 1 then
      log:warning('player perimeter point list is empty.  returning.')
      return nil
   end

   -- Compute the center of the hull.
   local center = Point3(0, 0, 0)
   for _, p in pairs(player_perimeter) do
      center = center + p
   end

   center = Point3(center.x / #player_perimeter, center.y / #player_perimeter, center.z / #player_perimeter)

   local remaining_tries = 10

   while remaining_tries > 0 do
      local rand_perimeter_point = player_perimeter[rng:get_int(1, #player_perimeter)]

      local spawn_dir_i = rand_perimeter_point - center
      local spawn_dir = Point3(spawn_dir_i.x, spawn_dir_i.y, spawn_dir_i.z)
      spawn_dir:normalize()
      spawn_dir:scale(distance)
      local scaled_spawn_dir = Point3(spawn_dir.x, spawn_dir.y, spawn_dir.z)
      local candidate_point = rand_perimeter_point + scaled_spawn_dir

      candidate_point = radiant.terrain.get_standable_point(entity, candidate_point)

      local direct_path_finder = _radiant.sim.create_direct_path_finder(entity)
                                    :set_start_location(candidate_point)
                                    :set_end_location(rand_perimeter_point)
      local path = direct_path_finder:get_path()
      if path then
         log:spam('Found candidate spawn location: %s', tostring(candidate_point))
         return candidate_point
      end
      log:spam('Bad candidate spawn location: %s', tostring(candidate_point))
      remaining_tries = remaining_tries - 1
   end

   -- If we've tried a couple of times and can't find one, give up.
   log:warning('Gave up trying to find a spawn location for entity: %s', tostring(entity))
   return nil
end


-- Find a point 'distance' units outside the perimeter of the explored area of the civilization 
-- to spawn an entity.
-- @param entity - the entity that we want to spawn
-- @param destination - the entity that the first entity should be able to path to
-- @param distance - the number of units outside the explored perimeter to spawn the critter
-- @param tries - how many times will we try before giving up?
-- @param max_steps - num "steps" the pathfinder is allowed to use to try to get this solution
-- @param success_cb - fn to run when we find a point, passes in the the spawn location
-- @param fail_cb - fn to run if we can't find a point
function SpawnRegionFinderService:find_point_outside_civ_perimeter_for_entity_astar(entity, destination_ent, distance, max_attempts, max_steps, success_cb, fail_cb)
   local remaining_tries = max_attempts
   local function try_spawn()
      log:spam('Looking for spawn location')

      -- Consult the convex hull of points that the civs have travelled.
      local player_perimeter = stonehearth.terrain:get_player_perimeter('civ')
      local candidate_point
      if #player_perimeter > 3 then
         candidate_point = _generate_random_point_outside_perimeter(player_perimeter, distance)
      else
         local theta = rng:get_real(0, math.pi * 2)
         local offset = Point3(distance * math.sin(theta), 0, distance * math.cos(theta))
         candidate_point = radiant.entities.get_world_grid_location(destination_ent) + offset:to_closest_int()
      end
      assert(candidate_point)

      candidate_point = radiant.terrain.get_standable_point(entity, candidate_point)

      local start_proxy = radiant.entities.create_entity()
      radiant.terrain.place_entity_at_exact_location(start_proxy, candidate_point)

      local path_finder = _radiant.sim.create_astar_path_finder(start_proxy, 'find spawn point outside civ perimeter')
                                    :add_destination(destination_ent)
                                    :set_max_steps(max_steps)

      local function cleanup()
         path_finder:destroy()
         radiant.entities.destroy_entity(start_proxy)
      end

      path_finder:set_solved_cb(function(path)
            log:spam('Found candidate spawn location: %s', tostring(candidate_point))
            success_cb(candidate_point)
            cleanup()
         end)
      path_finder:set_search_exhausted_cb(function()
            log:spam('Bad candidate spawn location: %s', tostring(candidate_point))
            cleanup()
            remaining_tries = remaining_tries - 1

            if remaining_tries == 0 then
               log:warning('Gave up trying to find a spawn location for entity: %s', tostring(entity))
               
               if fail_cb then
                  fail_cb()
               end
            else
               try_spawn()
            end
         end)
      path_finder:start()
   end
   try_spawn()
end


return SpawnRegionFinderService
