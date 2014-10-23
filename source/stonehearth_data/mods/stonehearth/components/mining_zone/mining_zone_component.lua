local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local TraceCategories = _radiant.dm.TraceCategories
local log = radiant.log.create_logger('mining')

local MiningZoneComponent = class()

local MAX_REACH_DOWN = 1
local MAX_REACH_UP = 3
local MAX_DESTINATION_DELTA_Y = 3

local FACE_DIRECTIONS = {
    Point3.unit_y,
   -Point3.unit_y,
    Point3.unit_z,
   -Point3.unit_z,
    Point3.unit_x,
   -Point3.unit_x
}

local NON_TOP_DIRECTIONS = {
    Point3.unit_z,
   -Point3.unit_z,
    Point3.unit_x,
   -Point3.unit_x,
   -Point3.unit_y
}

function MiningZoneComponent:__init()
end

function MiningZoneComponent:initialize(entity, json)
   self._entity = entity
   self._destination_component = self._entity:add_component('destination')

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._destination_component
         :set_region(_radiant.sim.alloc_region3())
         :set_adjacent(_radiant.sim.alloc_region3())
         :set_reserved(_radiant.sim.alloc_region3())
      self:set_region(_radiant.sim.alloc_region3())
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end

   self:_trace_reserved()

   -- TODO: listen for changes to terrain
end

function MiningZoneComponent:_restore()
   self:_trace_region()
   self:_create_mining_task()
end

function MiningZoneComponent:destroy()
   self:_destroy_region_trace()
   self:_destroy_reserved_trace()
end

-- region is a boxed region
function MiningZoneComponent:get_region()
   return self._sv.region
end

-- region is a boxed region
function MiningZoneComponent:set_region(region)
   self._sv.region = region
   self:_trace_region()
   self.__saved_variables:mark_changed()
   return self
end

function MiningZoneComponent:mine_point(point)
   radiant.terrain.subtract_point(point)
   self:_update_destination()

   if self._destination_component:get_region():get():empty() then
      radiant.entities.destroy_entity(self._entity)
   end
end

function MiningZoneComponent:_trace_region()
   self:_destroy_region_trace()

   self._region_trace = self._sv.region:trace('mining zone')
      :on_changed(function()
            self:_on_region_changed()
         end)
      :push_object_state()
end

function MiningZoneComponent:_destroy_region_trace()
   if self._region_trace then
      self._region_trace:destroy()
      self._region_trace = nil
   end
end

function MiningZoneComponent:_on_region_changed()
   -- cache the region bounds
   self._sv.region:modify(function(cursor)
         cursor:optimize_by_merge()
      end)
   self._sv.region_bounds = self._sv.region:get():get_bounds()
   self.__saved_variables:mark_changed()

   self:_update_destination()
   self:_create_mining_task()
end

function MiningZoneComponent:_trace_reserved()
   self:_destroy_reserved_trace()

   self._reserved_trace = self._destination_component:trace_reserved('mining zone component', TraceCategories.SYNC_TRACE)
      :on_changed(function()
            self:_on_reserved_changed()
         end)
end

function MiningZoneComponent:_destroy_reserved_trace()
   if self._reserved_trace then
      self._reserved_trace:destroy()
      self._reserved_trace = nil
   end
end

function MiningZoneComponent:_on_reserved_changed()
   self:_update_adjacent()
end

function MiningZoneComponent:_count_open_faces_for_block(point, max)
   max = max or 2
   local test_point = Point3()
   local count = 0

   for _, dir in ipairs(FACE_DIRECTIONS) do
      test_point:set(point.x+dir.x, point.y+dir.y, point.z+dir.z)

      if not radiant.terrain.is_terrain(test_point) then
         count = count + 1
         if count >= max then
            return count
         end
      end
   end

   return count
end

function MiningZoneComponent:_has_n_plus_exposed_faces(point, n)
   local count = self:_count_open_faces_for_block(point, n)
   return count >= n
end

function MiningZoneComponent:_top_face_exposed(point)
   local above_point = point + Point3.unit_y
   local exposed = not radiant.terrain.is_terrain(above_point)
   return exposed
end

function MiningZoneComponent:_non_top_face_exposed(point)
   local adjacent_point = Point3()

   for _, dir in ipairs(NON_TOP_DIRECTIONS) do
      adjacent_point:set(point.x+dir.x, point.y+dir.y, point.z+dir.z)

      if not radiant.terrain.is_terrain(adjacent_point) then
         return true
      end
   end
   return false
end

function MiningZoneComponent:_top_and_second_face_exposed(point)
   if not self:_top_face_exposed(point) then
      return false
   end

   -- look for a second exposed face
   local second_exposed_face = self:_non_top_face_exposed(point)
   return second_exposed_face
end

function MiningZoneComponent:_has_higher_neighbor(point, radius)
   local above_point = Point3()
   local y = point.y + 1
   
   for z = -radius, radius do
      for x = -radius, radius do
         if x ~= 0 or y ~= 0 then
            above_point:set(point.x+x, y, point.z+z)
            if radiant.terrain.is_terrain(above_point) then
               return true
            end
         end
      end
   end
   return false
end

function MiningZoneComponent:_each_corner_block_in_cube(cube, cb)
   -- block is a corner block when it is part of three or more faces
   self:_each_block_in_cube_with_faces(cube, 3, cb)
end

function MiningZoneComponent:_each_edge_block_in_cube(cube, cb)
   -- block is an edge block when it is part of two or more faces
   self:_each_block_in_cube_with_faces(cube, 2, cb)
end

function MiningZoneComponent:_each_face_block_in_cube(cube, cb)
   -- block is a face block when it is part of one or more faces
   self:_each_block_in_cube_with_faces(cube, 1, cb)
end

-- invokes callback when a block in the cube is on min_faces or more
-- min_faces = 3 iterates over all the corner blocks
-- min_faces = 2 iterates over all the edge blocks
-- min_faces = 1 iterates over all the face blocks
function MiningZoneComponent:_each_block_in_cube_with_faces(cube, min_faces, cb)
    -- subtract one because we want the max terrain block, not the bounding grid line
   local max = cube.max - Point3.one
   local min = cube.min
   local x, y, z, x_face, y_face, z_face, num_faces
   -- reuse this point to avoid inner loop memory allocation
   local point = Point3()

   local face_count = function(value, min, max)
      if value == min or value == max then
         return 1
      else
         return 0
      end
   end

   -- can't use for loops because lua doesn't respect changes to the loop counter in the optimization check
   y = min.y
   while y <= max.y do
      y_face = face_count(y, min.y, max.y)
      if y_face + 2 < min_faces then
         -- optimizaton, jump to opposite face if constraint cannot be met
         y = max.y
         y_face = 1
      end

      z = min.z
      while z <= max.z do
         z_face = face_count(z, min.z, max.z)
         if y_face + z_face + 1 < min_faces then
            -- optimizaton, jump to opposite face if constraint cannot be met
            z = max.z
            z_face = 1
         end

         x = min.x
         while x <= max.x do
            x_face = face_count(x, min.x, max.x)
            if x_face + y_face + z_face < min_faces then
               -- optimizaton, jump to opposite face if constraint cannot be met
               x = max.x
               x_face = 1
            end

            num_faces = x_face + y_face + z_face

            if num_faces >= min_faces then
               point:set(x, y, z)
               --log:spam('cube iterator: %s', point)
               cb(point)
            end
            x = x + 1
         end
         z = z + 1
      end
      y = y + 1
   end   
end

-- To make sure we mine in layers instead of digging randomly and potentially orphaning regions,
-- we prioritize blocks with two or more exposed faces.
function MiningZoneComponent:_update_destination()
   if self._sv.region_bounds:get_area() == 0 then
      return
   end

   local location = radiant.entities.get_world_grid_location(self._entity)
   local world_space_region = self._sv.region:get():translated(location)
   local terrain_region = radiant.terrain.intersect_region(world_space_region)
   terrain_region:set_tag(0)
   terrain_region:optimize_by_merge()
   local bounds = terrain_region:get_bounds()
   
   self._destination_component:get_region():modify(function(cursor)
         -- reuse this cube to avoid inner loop memory allocation
         local block = Cube3()
         local world_to_local_translation = -location
         cursor:clear()

         local add_block = function(point)
               block.min:set(point.x,   point.y,   point.z)
               block.max:set(point.x+1, point.y+1, point.z+1)
               block:translate(world_to_local_translation)
               cursor:add_cube(block)
            end

         for cube in terrain_region:each_cube() do
            -- this code not finalized
            if cube.max.y >= bounds.max.y - MAX_DESTINATION_DELTA_Y then
               local slice_min = Point3(cube.min.x, cube.max.y-1, cube.min.z)
               local slice_max = cube.max
               local top_slice = Cube3(slice_min, slice_max)

               if cube.max.y == bounds.max.y then
                  self:_each_face_block_in_cube(top_slice, function(point)
                        if self:_top_face_exposed(point) then
                           add_block(point)
                        end
                     end)
               else
                  self:_each_face_block_in_cube(top_slice, function(point)
                        if self:_top_face_exposed(point) and not self:_has_higher_neighbor(point, 1) then
                           add_block(point)
                        end
                     end)
               end
            end
         end

         if cursor:empty() then
            for cube in terrain_region:each_cube() do
               self:_each_edge_block_in_cube(cube, function(point)
                     -- prioritizing blocks with 2+ exposed faces naturally mines in layers
                     -- provided the starting shape is relatively flat
                     if self:_has_n_plus_exposed_faces(point, 2) then
                        add_block(point)
                     end
                  end)
            end
         end

         if cursor:empty() then
            for cube in terrain_region:each_cube() do
               self:_each_face_block_in_cube(cube, function(point)
                     if self:_has_n_plus_exposed_faces(point, 1) then
                        add_block(point)
                     end
                  end)
            end
         end

         -- mostly for debugging
         cursor:optimize_by_merge()
      end)

   self:_update_adjacent()
end

function MiningZoneComponent:_update_adjacent()
   local location = radiant.entities.get_world_grid_location(self._entity)
   local destination_region = self._destination_component:get_region():get()
   local reserved_region = self._destination_component:get_reserved():get()
   local unreserved_destination_region = destination_region - reserved_region

   self._destination_component:get_adjacent():modify(function(cursor)
         cursor:clear()

         for point in unreserved_destination_region:each_point() do
            local world_point = point + location
            local adjacent_region = stonehearth.mining:get_adjacent_for_destination_block(world_point)
            adjacent_region:translate(-location)
            cursor:add_region(adjacent_region)
         end

         cursor:optimize_by_merge()
      end)
end

function MiningZoneComponent:_create_mining_task()
   local town = stonehearth.town:get_town(self._entity)
   
   self:_destroy_mining_task()

   if self._sv.region:get():empty() then
      return
   end

   self._mining_task = town:create_task_for_group('stonehearth:task_group:mining',
                                                  'stonehearth:mining:mine',
                                                  {
                                                     mining_zone = self._entity
                                                  })
      :set_source(self._entity)
      :set_name('mine task')
      :set_priority(stonehearth.constants.priorities.mining.MINE)
      :set_max_workers(4)
      :notify_completed(function()
            self._mining_task = nil
         end)
      :start()
end

function MiningZoneComponent:_destroy_mining_task()
   if self._mining_task then
      self._mining_task:destroy()
      self._mining_task = nil
   end
end

return MiningZoneComponent
