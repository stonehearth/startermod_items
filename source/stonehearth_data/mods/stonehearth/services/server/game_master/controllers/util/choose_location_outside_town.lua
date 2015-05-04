local rng = _radiant.csg.get_default_rng()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local ChooseLocationOutsideTown = class()

--Choose a location outside town to create something
--@param player_id - id of the player being targeted by the object to be spawned
--@param min_range - minimum range at which to spawn the thing
--@param max_range - maximum range at which to spawn the thing
--@param callback - fn to call when we've got the region
--@param target_region - optional, area around the target location to ensure is clear, for placing items
function ChooseLocationOutsideTown:initialize(player_id, min_range, max_range, callback, target_region)
   checks('self', 'string', 'number', 'number', 'binding')

   self._sv.player_id = player_id
   self._sv.min_range = min_range
   self._sv.max_range = max_range
   self._sv.target_region = target_region
   self._sv.callback  = callback
   self._sv.slop_space = 1
   self._sv.found_location = false
end

function ChooseLocationOutsideTown:activate()
   self._log = radiant.log.create_logger('game_master')
                              :set_prefix('choose loc outside city')

   self:_destroy_find_location_timer()

   if not self._sv.found_location then
      self:_try_finding_location()
   end
end

function ChooseLocationOutsideTown:_try_location(location)
   if self._sv.target_region then
      --Check that there is no terrain above the surface of the region
      local test_region = self._sv.target_region:translated(location)
      local intersection = radiant.terrain.intersect_region(test_region)
      if not intersection:empty() then
         self._log:info('location %s intersects terrain (intersection:%s).  trying again', location, intersection:get_bounds())
         return false
      end

      --Check that the region is supported by terrain (translateD makes a copy of the region, yay!)
      intersection = radiant.terrain.intersect_region(test_region:translated(-Point3.unit_y))
      if intersection:get_area() ~= test_region:get_area() then
         self._log:info('location %s  not flat.  trying again (supported area:%d   test area:%d)', location,
                        intersection:get_area(), test_region:get_area())
         return false
      end
   end
   return true
end

function ChooseLocationOutsideTown:_try_finding_location()
   -- start a background job to find a location.
   self._job = radiant.create_background_task('choose location outside town', function()
         self:_try_finding_location_thread()
      end)
end

function ChooseLocationOutsideTown:_try_finding_location_thread()
   local player_banner = stonehearth.town:get_town(self._sv.player_id)
                                             :get_banner()
   local banner_location = radiant.entities.get_world_grid_location(player_banner)

   local open = {}
   local closed = {}

   -- if not already contained in `closed`, adds `pt` to `closed`, as
   -- well as `open` if `add_to_open` is true.
   local function visit_point(pt, add_to_open)
      checks('Point3', 'boolean')
      local key = pt:key_value()
      if not closed[key] then
         if add_to_open then
            table.insert(open, {
                  location = pt,
                  distance = pt:distance_to(banner_location)
               })
         end
         closed[key] = true
      end
   end

   -- return the best node in `open` where "best" is defined as the one
   -- that's furthest away
   local function get_first_open_node()
      -- this is O(n) + a possible O(n) compaction, but that's still better
      -- than O(n lg n) sorting!
      local best_i, best_node, best_distance
      for i, node in ipairs(open) do
         if not best_distance or node.distance > best_distance then
            best_i, best_node, best_distance = i, node, node.distance
         end
      end
      if best_i then
         table.remove(open, best_i)
         return best_node
      end
   end

   -- figure out how far out we should look.  we increase the tolerance
   -- by a little bit each time through.
   local d = rng:get_int(self._sv.min_range, self._sv.max_range)
   local min_range = d - self._sv.slop_space
   local max_range = d + self._sv.slop_space

   -- start at a random point
   local convex_hull = stonehearth.terrain:get_player_perimeter('civ')
   if convex_hull then
      local starting_point = convex_hull[rng:get_int(1, #convex_hull)]
      visit_point(starting_point, true)
   else
      visit_point(banner_location, true)
   end

   -- run through every point on this plane looking for a good one!
   while #open > 0 do
      local node = get_first_open_node()
      if node.distance >= min_range then
         if self:_try_location(node.location) then
            --if everything is fine, succeed!
            self._log:info('found location %s', node.location)
            self:_finalize_location(node.location)                  
            return
         end
      end
      -- look at all 8 points around this one.
      for y=-1,1 do
         for x=-1,1 do
            for z=-1,1 do
               local next_pt = node.location + Point3(x, y, z)
               local add_to_open = false
               if node.distance < self._sv.max_range then
                  add_to_open = radiant.terrain.is_standable(next_pt)
               end
               visit_point(next_pt, add_to_open)
            end
         end
      end
   end
   self._log:info('failed to find location.  trying again')
   self:_try_later()
end

function ChooseLocationOutsideTown:_finalize_location(location)
   self:_destroy_find_location_timer()
 
   self._sv.found_location = true
   self.__saved_variables:mark_changed()
   if self._sv.callback then
      radiant.invoke(self._sv.callback, location)
      self._sv.callback = nil
   end
end

function ChooseLocationOutsideTown:_destroy_find_location_timer()
   if self._find_location_timer then
      self._find_location_timer:destroy()
      self._find_location_timer = nil
   end
end

function ChooseLocationOutsideTown:_try_later()
   self._sv.slop_space = self._sv.slop_space + 1
   self:_destroy_find_location_timer()
   self._find_location_timer = radiant.set_realtime_timer(5000, function()
         self:_try_finding_location()
      end)
end

return ChooseLocationOutsideTown
