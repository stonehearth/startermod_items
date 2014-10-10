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

-- To make sure we mine in layers instead of digging randomly and potentially orphaning regions,
-- we only allow blocks with two or more exposed faces to be part of the destination.
-- (A block in the underlying layer can have at most 1 exposed face. Note that multiple sides
-- of the region can qualify for the "top" layer and be simultaneously minable. This algorithm
-- also works for complex non-convex regions too!)
-- Conveniently, blocks with two or more exposed faces must be on the on the edge or corner or
-- a cube3. We prioritize corners over edges because this is the 2d analog to the 3d case and
-- forces the 2d surface to be mined in layers from the outside in.
-- This algorithm works much better than assigning a fixed mining direction which only works
-- for the simplest cases.
function MiningZoneComponent:_update_destination()
   if self._sv.region_bounds:get_area() == 0 then
      return
   end
   local location = radiant.entities.get_world_grid_location(self._entity)

   local block_has_multiple_open_faces = function(point)
      local world_point = point + location
      local test_point = Point3()
      local count = 0

      for _, dir in ipairs(FACE_DIRECTIONS) do
         test_point:set(world_point.x+dir.x, world_point.y+dir.y, world_point.z+dir.z)

         if not radiant.terrain.is_terrain(test_point) then
            count = count + 1
            if count >= 2 then
               return true
            end
         end
      end

      return false
   end

   self._destination_component:get_region():modify(function(cursor)
         cursor:clear()

         local world_space_region = self._sv.region:get():translated(location)
         local terrain_region = radiant.terrain.intersect_region(world_space_region)
         terrain_region:set_tag(0)
         terrain_region:optimize_by_merge()
         terrain_region:translate(-location)

         for cube in terrain_region:each_cube() do
            self:_add_cube_corners_to_region(cursor, cube, block_has_multiple_open_faces)
         end

         if cursor:empty() then
            -- add the whole region and wait for an adjacent to open up
            cursor:add_region(terrain_region)
         end
      end)

   self:_update_adjacent()
end

function MiningZoneComponent:_add_cube_corners_to_region(region, cube, filter_fn)
   local inc = cube:get_size() - Point3(1, 1, 1)
   local min = cube.min
   local max = min + inc
   local corner_cube = Cube3()

   -- if inc.dimension is 0, skip the max cube because the min and max cubes have the same coordinates
   -- e.g. for the unit cube:
   --   Cube3(cube.min, cube.min + Point3(1,1,1)) == Cube3(cube.max - Point3(1,1,1), cube.max)

   for y = min.y, max.y, inc.y do
      for z = min.z, max.z, inc.z do
         for x = min.x, max.x, inc.x do
            corner_cube.min:set(x, y, z)
            corner_cube.max:set(x+1, y+1, z+1)

            if filter_fn(corner_cube.min) then
               region:add_cube(corner_cube)
            end

            if inc.x == 0 then break end
         end
         if inc.z == 0 then break end
      end
      if inc.y == 0 then break end
   end
end

function MiningZoneComponent:_update_adjacent()
   local location = radiant.entities.get_world_grid_location(self._entity)
   local destination_region = self._destination_component:get_region():get()

   self._destination_component:get_adjacent():modify(function(cursor)
         -- reusing cube so we don't spam malloc
         local adj_cube = Cube3()
         cursor:clear()

         for point in destination_region:each_point() do
            -- TODO: define conditions for MAX_REACH_UP
            local y_min = point.y - MAX_REACH_UP
            local y_max = point.y + 1
            local world_point = point + location

            if not radiant.terrain.is_terrain(world_point + Point3.unit_y) then
               -- we can strike down on the adjacent block if nothing is on top of it
               y_max = y_max + MAX_REACH_DOWN
            end

            self:_set_adjacent_column(adj_cube, point.x-1, point.z, y_min, y_max)
            cursor:add_cube(adj_cube)

            self:_set_adjacent_column(adj_cube, point.x+1, point.z, y_min, y_max)
            cursor:add_cube(adj_cube)

            self:_set_adjacent_column(adj_cube, point.x, point.z-1, y_min, y_max)
            cursor:add_cube(adj_cube)

            self:_set_adjacent_column(adj_cube, point.x, point.z+1, y_min, y_max)
            cursor:add_cube(adj_cube)
         end

         cursor:optimize_by_merge()
      end)
end

function MiningZoneComponent:_set_adjacent_column(cube, x, z, y_min, y_max)
   cube.min:set(x, y_min, z)
   cube.max:set(x+1, y_max, z+1)
end

function MiningZoneComponent:_create_mining_task()
   local town = stonehearth.town:get_town(self._entity)
   
   self:_destroy_mining_task()

   if self._sv.region:get():empty() then
      return
   end

   -- TODO: decide how many tasks to manage
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
