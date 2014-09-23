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

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._entity:add_component('destination')
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

   -- TODO: mine point by point
   -- local location = radiant.entities.get_world_grid_location(self._entity)
   -- local world_region = self._sv.region:get():translated(location)
   -- radiant.terrain.subtract_region(world_region)

   -- TODO: if no blocks left to mine, destroy zone
   --radiant.entities.destroy_entity(self._entity)
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
   self._sv.region_bounds = self._sv.region:get():get_bounds()
   -- TODO: temporary code
   self._sv.current_slice = self._sv.region_bounds.max.y - 1
   self.__saved_variables:mark_changed()

   self:_update_destination()
   self:_create_mining_task()
end

function MiningZoneComponent:_update_destination()
   local location = radiant.entities.get_world_grid_location(self._entity)

   self._entity:add_component('destination'):get_region():modify(function(cursor)
         while true do
            cursor:clear()
            local working_bounds = self:_get_working_bounds()

            for cube in working_bounds:each_cube() do
               -- convert to world coordinates
               cube:translate(location)
               -- get affected terrain blocks
               local terrain_region = radiant.terrain.intersect_cube(cube)
               -- back to local coordinates
               terrain_region:translate(-location)
               cursor:add_region(terrain_region)
            end

            if not cursor:empty() then
               break
            else
               self._sv.current_slice = self._sv.current_slice - 1
               if self._sv.current_slice < self._sv.region_bounds.min.y then
                  -- TODO: we're done, delete the entity
                  break
               end
            end
         end
      end)

   self:_update_adjacent()
end

function MiningZoneComponent:_update_adjacent()
   local destination_region = self._entity:add_component('destination'):get_region():get()

   self._entity:add_component('destination'):get_adjacent():modify(function(cursor)
         -- reusing cube so we don't spam malloc
         local adj_cube = Cube3()
         cursor:clear()

         for dst_cube in destination_region:each_cube() do
            for point in dst_cube:each_point() do
               self:_set_adjacent_column(adj_cube, point.x-1, point.y, point.z)
               cursor:add_cube(adj_cube)

               self:_set_adjacent_column(adj_cube, point.x+1, point.y, point.z)
               cursor:add_cube(adj_cube)

               self:_set_adjacent_column(adj_cube, point.x, point.y, point.z-1)
               cursor:add_cube(adj_cube)

               self:_set_adjacent_column(adj_cube, point.x, point.y, point.z+1)
               cursor:add_cube(adj_cube)
            end
         end
      end)
end

function MiningZoneComponent:_set_adjacent_column(cube, x, y, z)
   cube.min.x, cube.min.z = x, z
   cube.max.x, cube.max.z = x + 1, z + 1
   cube.min.y = y - MAX_REACH_UP
   cube.max.y = y + MAX_REACH_DOWN + 1
end

function MiningZoneComponent:_get_working_bounds()
   local region = self._sv.region:get()
   local bounds = self._sv.region_bounds

   -- TODO: temporary code
   local slice = self._sv.current_slice
   local clipper = Region3(Cube3(
         Point3(bounds.min.x, slice, bounds.min.z),
         Point3(bounds.max.x, slice + 1, bounds.max.z)
      ))
   local working_bounds = region:intersected(clipper)
   return working_bounds
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
