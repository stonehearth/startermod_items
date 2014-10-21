local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('mining')

local MiningZoneComponent = class()

local MAX_REACH_DOWN = 1
local MAX_REACH_UP = 3

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

   -- TODO: listen for changes to terrain
end

function MiningZoneComponent:_restore()
   self:_create_mining_task()
end

function MiningZoneComponent:destroy()
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
   if self._region_trace then
      self._region_trace:destroy()
   end

   self._region_trace = self._sv.region:trace('mining zone')
      :on_changed(function()
            self:_on_region_changed()
         end)
      :push_object_state()
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

local FACE_DIRECTIONS = {
    Point3.unit_y,
   -Point3.unit_y,
    Point3.unit_z,
   -Point3.unit_z,
    Point3.unit_x,
   -Point3.unit_x
}

function MiningZoneComponent:_count_open_faces_for_block(point, max)
   max = max or 2
   local test_point = Point3()
   local count = 0

   for _, dir in ipairs(FACE_DIRECTIONS) do
      test_point:set(point.x+dir.x, point.y+dir.y, point.z+dir.z)

      -- TODO: consider testing for obstruction
      if not radiant.terrain.is_terrain(test_point) then
         count = count + 1
         if count >= max then
            return count
         end
      end
   end

   return count
end

function MiningZoneComponent:_top_face_exposed(point)
   local above_point = point + Point3.unit_y
   local exposed = not radiant.terrain.is_terrain(above_point)
   return exposed
end

local NON_TOP_DIRECTIONS = {
    Point3.unit_z,
   -Point3.unit_z,
    Point3.unit_x,
   -Point3.unit_x,
   -Point3.unit_y
}

function MiningZoneComponent:_non_top_face_exposed(point)
   local other_point = Point3()

   for _, dir in ipairs(NON_TOP_DIRECTIONS) do
      other_point:set(point.x+dir.x, point.y+dir.y, point.z+dir.z)

      if not radiant.terrain.is_terrain(other_point) then
         return true
      end
   end
   return false
end

function MiningZoneComponent:_is_priority_block(point)
   if not self:_top_face_exposed(point) then
      return false
   end

   -- look for a second exposed face
   return self:_non_top_face_exposed(point)
end

function MiningZoneComponent:_each_corner_block_in_cube(cube, cb)
   -- subtract one because we want the max terrain block, not the bounding grid line
   local max = cube.max - Point3.one
   local min = cube.min
   local inc = max - min
   -- reuse this point to avoid inner loop memory allocation
   local point = Point3()

   -- if inc.dimension is 0, skip the max cube because the min and max cubes have the same coordinates
   -- e.g. for the unit cube:
   --   Cube3(cube.min, cube.min + Point3(1,1,1)) == Cube3(cube.max - Point3(1,1,1), cube.max)

   for y = min.y, max.y, inc.y do
      for z = min.z, max.z, inc.z do
         for x = min.x, max.x, inc.x do
            point:set(x, y, z)
            cb(point)

            if inc.x == 0 then break end
         end
         if inc.z == 0 then break end
      end
      if inc.y == 0 then break end
   end
end

function MiningZoneComponent:_each_edge_block_in_cube(cube, cb)
    -- subtract one because we want the max terrain block, not the bounding grid line
   local max = cube.max - Point3.one
   local min = cube.min
   local x_face, y_face, z_face, num_faces
   -- reuse this point to avoid inner loop memory allocation
   local point = Point3()

   local face_count = function(value, min, max)
      if value == min or value == max then
         return 1
      else
         return 0
      end
   end

   for y = min.y, max.y do
      y_face = face_count(y, min.y, max.y)
      for z = min.z, max.z do
         z_face = face_count(z, min.z, max.z)
         for x = min.x, max.x do
            x_face = face_count(x, min.x, max.x)
            num_faces = x_face + y_face + z_face

            -- block is an edge block when it is part of two or more faces
            if num_faces >= 2 then
               point:set(x, y, z)
               cb(point)
            end
         end
      end
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

         -- add high priority blocks
         for cube in terrain_region:each_cube() do
            self:_each_edge_block_in_cube(cube, function(point)
                  if self:_is_priority_block(point) then
                     add_block(point)
                  end
               end)
         end

         -- add the edges of the zone
         -- local zone_region = self:get_region():get():translated(location)
         -- for cube in zone_region:each_cube() do
         --    self:_each_edge_block_in_cube(cube, function(point)
         --          if radiant.terrain.is_terrain(point) and self:_top_face_exposed(point) then
         --             add_block(point)
         --          end
         --       end)
         -- end

         if cursor:empty() then
            -- add the whole region
            terrain_region:translate(world_to_local_translation)
            cursor:add_region(terrain_region)
         else
            -- mostly for debugging
            cursor:optimize_by_merge()
         end
      end)

   self:_update_adjacent()
end

function MiningZoneComponent:_update_adjacent()
   local location = radiant.entities.get_world_grid_location(self._entity)
   local destination_region = self._destination_component:get_region():get()

   self._destination_component:get_adjacent():modify(function(cursor)
         cursor:clear()

         for point in destination_region:each_point() do
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
