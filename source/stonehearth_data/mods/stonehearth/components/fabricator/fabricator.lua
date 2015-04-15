local build_util = require 'lib.build_util'
local priorities = require('constants').priorities.simple_labor
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Color3 = _radiant.csg.Color3
local TraceCategories = _radiant.dm.TraceCategories

local emptyRegion = Region3()
local COORD_MAX = 1000000 -- 1 million enough?
local ADJACENT_POINTS = {
   Point3(-1, 0, -1),
   Point3(-1, 0,  1),
   Point3( 1, 0, -1),
   Point3( 1, 0,  1),
}

local Fabricator = class()

-- build the inverse material map
local COLOR_TO_MATERIAL = {}
local function build_color_to_material_map()  
   for material, colorlist in pairs(stonehearth.constants.construction.brushes.voxel) do
      for _, color in ipairs(colorlist) do
         if COLOR_TO_MATERIAL[color] then
            radiant.error('duplicate color %s in stonehearth:build:brushes', color)
         end
         COLOR_TO_MATERIAL[color] = material
      end
   end
end

-- this is quite annoying.  the order of operations during loading means that the fabricator for
-- an entity may be required before the stonehearth mod gets loaded.  if that happens, wait for
-- the load message before trying to rebuild the colormap
if rawget(_G, 'stonehearth') then
   build_color_to_material_map()
else
   radiant.events.listen_once(radiant, 'radiant:game_loaded',  build_color_to_material_map)
end

-- this is the component which manages the fabricator entity.
function Fabricator:__init(name, entity, blueprint, project)
   self.name = name

   self._log = radiant.log.create_logger('build.fabricator')
                        :set_prefix('fab for ' .. tostring(blueprint))
                        :set_entity(entity)

   self._log:debug('creating fabricator')
   
   self._finished = false
   self._entity = entity
   self._fabricator_dst = self._entity:add_component('destination')
   self._fabricator_rcs = self._entity:add_component('region_collision_shape')
   self._blueprint = blueprint
   self._blueprint_dst = blueprint:get_component('destination')
   self._blueprint_construction_data = blueprint:get_component('stonehearth:construction_data')
   self._blueprint_construction_progress = blueprint:get_component('stonehearth:construction_progress')
   self._mining_zones = {}
   self._mining_traces = {}
   self._total_mining_region = _radiant.sim.alloc_region3()

   self._traces = {}
   self._active = false

   assert(self._blueprint_construction_data)
   assert(radiant.entities.get_player_id(self._blueprint) ~= '')

   self._resource_material = self._blueprint_construction_data:get_material()
   if project then
      self:_initialize_existing_project(project)
   else
      self:_create_new_project()
   end

   local update_dst_adjacent = function()
      self:_update_dst_adjacent()
   end
   local update_dst_region = function()
      self:_update_dst_region()
   end

   table.insert(self._traces, self._fabricator_rcs:trace_region('fabricator dst region', TraceCategories.SYNC_TRACE):on_changed(update_dst_region))
   table.insert(self._traces, self._fabricator_dst:trace_region('fabricator dst adjacent', TraceCategories.SYNC_TRACE):on_changed(update_dst_adjacent))
   table.insert(self._traces, self._fabricator_dst:trace_reserved('fabricator dst reserved', TraceCategories.SYNC_TRACE):on_changed(update_dst_adjacent))
   
   self:_trace_blueprint_and_project()

   self._finished_listener = radiant.events.listen(self._blueprint, 'stonehearth:construction:dependencies_finished_changed', self, self._on_can_start_changed)
   self:_on_can_start_changed()
end

function Fabricator:destroy()
   self._log:debug('destroying fabricator')

   if self._finished_listener then
      self._finished_listener:destroy()
      self._finished_listener = nil
   end

   for _, trace in ipairs(self._traces) do
      trace:destroy()
   end
   self._traces = {}
   self:_stop_project()

   if self._project then
      radiant.entities.destroy_entity(self._project)
      self._project = nil
   end
end

function Fabricator:set_scaffolding(scaffolding)
   self._scaffolding = scaffolding
end

function Fabricator:set_active(active)
   self._active = active
   if self._active then
      self:_on_can_start_changed()
      self:_start_project()
   else
      self:_stop_project()
   end
end

function Fabricator:set_teardown(teardown)
   self._teardown = teardown
   if self._active then
      self:_update_fabricator_region()
   end
end

function Fabricator:_initialize_existing_project(project)  
   self._project = project
   self._project_dst = self._project:get_component('destination')

   self._fabricator_dst:get_reserved():modify(function(cursor)
      cursor:clear()
   end)
end

function Fabricator:instabuild()
   radiant.terrain.subtract_region(self._total_mining_region:get())
   self._project_dst:get_region():modify(function(cursor)
         cursor:copy_region(self._blueprint_dst:get_region():get())
      end)
end

function Fabricator:_create_new_project()
   -- initialize the fabricator entity.  we'll manually update the
   -- adjacent region of the destination so we can build the project
   -- in layers.  this helps prevent the worker from getting stuck
   -- and just looks cooler
   self._fabricator_dst:set_region(_radiant.sim.alloc_region3())
            :set_reserved(_radiant.sim.alloc_region3())
            :set_adjacent(_radiant.sim.alloc_region3())

   self._fabricator_rcs:set_region(_radiant.sim.alloc_region3())
                       :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)

       
   -- create a new project.  projects start off completely unbuilt.
   -- projects are stored in as children to the fabricator, so there's
   -- no need to update their transform.
   local blueprint = self._blueprint
   local rgn = _radiant.sim.alloc_region3()
   self._project = radiant.entities.create_entity(blueprint:get_uri(), { owner = self._entity })
   self._project:set_debug_text('project')
   self._project_dst = self._project:add_component('destination')

   radiant.entities.set_player_id(self._project, blueprint)

   self._project_dst:set_region(rgn)
   self._project:add_component('region_collision_shape')
                     :set_region(rgn)

   local mob = self._entity:get_component('mob')
   local parent = mob:get_parent()
   assert(parent)
   parent:add_component('entity_container'):add_child(self._project)
   self._project:add_component('mob'):set_transform(mob:get_transform())
                        
   -- get fabrication specific info, if available.  copy it into the project, too
   -- so everything gets rendered correctly.
   local building = build_util.get_building_for(blueprint)
   local state = self._blueprint_construction_data:get_savestate()
   self._project:add_component('stonehearth:construction_data', state)
end

function Fabricator:_on_can_start_changed()
   if not self._active then
      return
   end
   local can_start = build_util.can_start_building(self._blueprint)
   if can_start ~= self._can_start then
      self._can_start = can_start
      self._log:debug('got stonehearth:construction:dependencies_finished_changed event (dependencies finished = %s)',
                      tostring(self._can_start))

      self:_start_project()
   end
end

function Fabricator:get_material(world_location)
   checks('self', 'Point3')

   if self._resource_material then
      -- for example, the scaffolding is made of wood (period).  this is only
      -- used by the scaffolding code
      self._log:spam('returning cd material "%s"', self._resource_material)
      return self._resource_material
   end

   local offset = radiant.entities.world_to_local(world_location, self._entity)
   local tag = self._blueprint_dst
                        :get_region()
                           :get()
                              :get_tag(offset)

   local color = Color3(tag)
   local material = COLOR_TO_MATERIAL[tostring(color)]
   if not material then
      radiant.error("building color to material map has no entry for color %s", tostring(color))
   end
   self._log:detail('returning material "%s" for block at %s (color:%s)', material, offset, color)
   return material
end

function Fabricator:get_project()
   return self._project
end

function Fabricator:get_blueprint()
   return self._blueprint
end

function Fabricator:get_entity()  
   return self._entity
end

function Fabricator:get_total_mining_region()
   return self._total_mining_region
end

function Fabricator:add_block(material_entity, location)  
   if not self._entity:is_valid() then
      return false
   end
   
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building  
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

   self._log:info('adding block %s', pt)

   -- if we've projected the fabricator region to the base of the project,
   -- the location passed in will be at the base, too.  find the appropriate
   -- block to add to the project collision shape by starting at the bottom
   -- and looking up.
   if self._blueprint_construction_data:get_project_adjacent_to_base() then
      local project_rgn = self._project_dst:get_region():get()
      local blueprint_rgn = self._blueprint_dst:get_region():get()
      local blueprint_top = blueprint_rgn:get_bounds().max.y

      -- we're looking for the bottom most block that we can build.  keep
      -- walking the point up until we find something which is both
      -- inside the blueprint and not in the project.
      while true do
         if pt.y > blueprint_top then
            self._log:warning('grew point %s outside blueprint bounds.  bailing', pt)
            break
         end

         if blueprint_rgn:contains(pt) then
            if not project_rgn:contains(pt) then
               -- yay!  this is the point we need to build
               break
            end
         end
         pt.y = pt.y + 1
      end

      if not blueprint_rgn:contains(pt) then
         -- we couldn't find a block in this column that is both missing from the
         -- project and inside the blueprint.  we must already be done!!
         self._log:info('skipping location %s -> %s.  no longer in blueprint!', location, pt)
         self:_log_destination(self._entity)
         self:_log_destination(self._blueprint)
         self:_log_destination(self._project)
         return
      end
   end

   self._project_dst:get_region():modify(function(cursor)
         local color = self._blueprint_dst
                              :get_region()
                                 :get()
                                    :get_tag(pt)
         cursor:add_point(pt, color)
         cursor:optimize_by_merge('add block in project')
      end)
   self:release_block(location)
   return true
end

function Fabricator:find_another_block(carrying, location)
   --[[
   if self._should_teardown then
      return nil
   end
   ]]
   
   if carrying then
      local origin = radiant.entities.get_world_grid_location(self._entity)
      local pt = location - origin
      
      local adjacent = self._fabricator_dst:get_adjacent():get()
      if adjacent:contains(pt) then
         local poi_local = self._fabricator_dst:get_point_of_interest(pt)
         if poi_local then
            local block = poi_local + origin

            local material = self:get_material(block)
            if not radiant.entities.is_material(carrying, material) then
               return
            end

            -- make sure the next block we get is on the same level as the current
            -- block so we don't confuse the scaffolding fabricator.  if we really
            -- need to go to the next row, the pathfinder will take us there!
            if block.y == location.y then
               self:reserve_block(block)
               return block
            end
         end
      end
   end
end

function Fabricator:remove_block(location)
   if not self._entity:is_valid() then
      return false
   end
   
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

   if not self._fabricator_dst:get_region():get():contains(pt) then
      self._log:warning('trying to remove unbuild portion %s of construction project', pt)
      return false
   end
   local project_rgn = self._project_dst:get_region():get()
   if not project_rgn:contains(pt) then
      self._log:warning('trying to remove unbuild portion %s of construction project', pt)
      return false
   end

   -- if we've projected the fabricator region to the base of the project,
   -- the location passed in will be at the base, too.  find the appropriate
   -- block to remove to the project collision shape by starting at the tip
   -- top and looking down
   if self._blueprint_construction_data:get_project_adjacent_to_base() then
      local bounds = project_rgn:get_bounds()
      local top = bounds.max.y - 1
      local bottom = bounds.min.y
      while top >= bottom do
         pt.y = top
         if project_rgn:contains(pt) then
            break
         end
         top = top - 1
      end
      if not project_rgn:contains(pt) then
         assert(false, 'could not compute block to teardown from projected adjacent')
      end
   end

   self._project_dst:get_region():modify(function(cursor)
      cursor:subtract_point(pt)
   end)
   return true
end

function Fabricator:_start_project()
   -- If we're tearing down the project, we only need to start the teardown
   -- task.  If we're building up and all our dependencies are finished
   -- building up, start the pickup and fabricate tasks
   self._log:detail('start_project (active:%s can_start:%s teardown:%s finished:%s)',
                     tostring(self._active), tostring(self._can_start), tostring(self._should_teardown), tostring(self._finished))
   if self._finished then
      return
   end

   local active = self._active and self._can_start


   -- Now apply the deltas.  Create tasks that need creating and destroy
   -- ones that need destroying.
   if active then
      if self._should_teardown then
         self:_destroy_fabricate_task()
         self:_start_teardown_task()
      else
         self:_destroy_teardown_task()
         self:_start_fabricate_task()
      end
   else
      self:_destroy_teardown_task()
      self:_destroy_fabricate_task()
   end

   if self._scaffolding then
      self._scaffolding:set_active(active)
   end
end

function Fabricator:_start_teardown_task()
   if not self._teardown_task then
      self._log:debug('starting teardown task')
      local town = stonehearth.town:get_town(self._project)
      self._teardown_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:teardown_structure', { fabricator = self })
                                      :set_name('fabricate ' .. tostring(self._blueprint))
                                      :set_source(self._entity)
                                      :set_max_workers(self:get_max_workers())
                                      :set_priority(priorities.TEARDOWN_BUILDING)
                                      :set_affinity_timeout(10)
                                      :start()
   end
end

function Fabricator:_destroy_teardown_task()
   if self._teardown_task then
      self._log:debug('destroying teardown task')
      self._teardown_task:destroy()
      self._teardown_task = nil
   end
end

function Fabricator:_start_fabricate_task()
   if not self._fabricate_task then
      self:_update_mining_region()
      self._log:debug('starting fabricate task')
      local town = stonehearth.town:get_town(self._blueprint)
      self._fabricate_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:fabricate_structure', { fabricator = self })
                                      :set_name('fabricate ' .. tostring(self._blueprint))
                                      :set_source(self._entity)
                                      :set_max_workers(self:get_max_workers())
                                      :set_priority(priorities.CONSTRUCT_BUILDING)
                                      :set_affinity_timeout(10)
                                      :start()
   end
end

function Fabricator:_destroy_fabricate_task()
   if self._fabricate_task then
      self._log:debug('destroying fabricate task')
      self._fabricate_task:destroy()
      self._fabricate_task = nil
   end      
end

function Fabricator:needs_work()
   return self._fabricate_task ~= nil
end

function Fabricator:needs_teardown()
   return self._teardown_task ~= nil
end

function Fabricator:reserve_block(location)
   local pt = location - radiant.entities.get_world_grid_location(self._entity)
   self._log:debug('adding point %s to reserve region', tostring(pt))
   self._fabricator_dst:get_reserved():modify(function(cursor)
      cursor:add_point(pt)
   end)
   self._log:debug('finished adding point %s to reserve region', tostring(pt))
end

function Fabricator:release_block(location)
   if not self._entity:is_valid() then
      return
   end

   local pt = location - radiant.entities.get_world_grid_location(self._entity)
   self._log:debug('removing point %s from reserve region', tostring(pt))
   self._fabricator_dst:get_reserved():modify(function(cursor)
      cursor:subtract_point(pt)
   end)
   self._log:debug('finished removing point %s from reserve region', tostring(pt))
end

function Fabricator:get_max_workers()
   return self._blueprint_construction_data:get_max_workers()
end

function Fabricator:_stop_project()
   self._log:warning('fabricator stopping all worker tasks')
   if self._teardown_task then
      self._teardown_task:destroy()
      self._teardown_task = nil
   end
   if self._fabricate_task then
      self._fabricate_task:destroy()
      self._fabricate_task = nil
   end

   for id, mining_zone in pairs(self._mining_zones) do
      -- xxx: this is could cause some trouble... what happens if the mining zone
      -- got merged with some other mining request?  didn't we just kill that one,
      -- too?
      radiant.entities.destroy_entity(mining_zone)
      self._mining_traces[id]:destroy()
      self._mining_zones[id] = nil
      self._mining_traces[id] = nil
   end
end

function Fabricator:_update_dst_region()
   local rcs_rgn = self._fabricator_rcs:get_region():get():to_int()
   local reserved_rgn = self._fabricator_dst:get_reserved():get()
   
   -- build in layers.  stencil out all but the bottom layer of the 
   -- project.  work against the destination region (not rgn - reserved).
   -- otherwise, we'll end up moving to the next row as soon as the last
   -- block in the current row is reserved.
   local bottom = rcs_rgn:get_bounds().min.y
   local clipper = Region3(Cube3(Point3(-COORD_MAX, bottom + 1, -COORD_MAX),
                                 Point3( COORD_MAX, COORD_MAX,   COORD_MAX)))
   local dst_region = rcs_rgn - clipper
   dst_region:set_tag(0)

   -- some projects want the worker to stand at the base of the project and
   -- push columns up.  for example, scaffolding always gets built from the
   -- base.  if this is one of those, translate the adjacent region all the
   -- way to the bottom.
   if self._blueprint_construction_data:get_project_adjacent_to_base() then
      -- reference the proper source for where the columns are based on whether
      -- we're tearing down or building up
      local shape_region
      if self._should_teardown then
         shape_region = self._project_dst:get_region():get()
      else
         shape_region = self._blueprint_dst:get_region():get()
      end

      -- project each column down to the base
      local dr = Region3()      
      local bottom = shape_region:get_bounds().min.y
      for pt in dst_region:each_point() do
         pt.y = bottom
         while not shape_region:contains(pt) do
            pt.y = pt.y + 1
         end
         dr:add_unique_point(pt)
      end
      dst_region = dr
   end
   dst_region:optimize_by_merge('fabricator dst')

   --self._log:detail('update dst region')
   --self:_log_region(rcs_rgn, 'region collision shape ->')
   --self:_log_region(dst_region, 'resulted in destination ->')
   
   -- Any region that needs mining should be removed from our destination region.
   for id, mining_zone in pairs(self._mining_zones) do
      local mining_region = mining_zone:get_component('destination')
                                          :get_region()
                                             :get()
      if not mining_region:empty() then
         local offset = radiant.entities.get_world_grid_location(self._entity) -
                        radiant.entities.get_world_grid_location(mining_zone)

         local fab_mining_region = mining_region:translated(offset)

         dst_region:subtract_region(fab_mining_region)
      end
   end

   -- copy into the destination region
   self._fabricator_dst:get_region():modify(function (cursor)
         cursor:copy_region(dst_region)
      end)
end

function Fabricator:_update_dst_adjacent()
   local dst_rgn = self._fabricator_dst:get_region():get()
   local reserved_rgn = self._fabricator_dst:get_reserved():get()
   local available = dst_rgn - reserved_rgn
   
   local allow_diagonals = self._blueprint_construction_data:get_allow_diagonal_adjacency()
   local adjacent = available:get_adjacent(allow_diagonals)

   if self._blueprint_construction_data:get_allow_crouching_construction() then
      adjacent:add_region(adjacent:translated(Point3(0, 1, 0)))
   end
   
   -- remove parts of the adjacent region which overlap the fabrication region.
   -- otherwise we get weird behavior where one worker can build a block right
   -- on top of where another is standing to build another block, or workers
   -- can build blocks to hoist themselves up to otherwise unreachable spaces,
   -- getting stuck in the process
   adjacent:subtract_region(dst_rgn)
   
   -- now subtract out all the blocks that we've reserved
   -- XXX: i have no idea why we do this, but am too afraid to touch it.
   -- it sounds "more reasonable" to compute the adjacent region based 
   -- on the destination region - the reserved region, right?  or is
   -- the idea that we shouldn't allow people to stand in spots that are
   -- slated to be built?  if so, we need to subtract out all the standable
   -- points adjacent to the reserved, not the actual reserved region.
   -- le sigh. -- tony
   adjacent:subtract_region(reserved_rgn)
   
   -- self._log:detail('update dst adjacent')
   -- self:_log_region(dst_rgn, 'destination region ->')
   -- self:_log_region(adjacent, 'resulted in adjacent->')
   
   -- finally, copy into the adjacent region for our destination
   self._fabricator_dst:get_adjacent():modify(function(cursor)
      cursor:copy_region(adjacent)
   end)
end

-- Called only when the blueprint region is changed (added or merged).
function Fabricator:_update_mining_region()
   local world_region = radiant.entities.local_to_world(self._blueprint_dst:get_region():get(), self._entity)
   world_region = radiant.terrain.intersect_region(world_region)

   if world_region:empty() then
      return
   end

   -- The mining service will handle all existing mining region overlap merging for us.
   local player_id = radiant.entities.get_player_id(self._blueprint)
   local mining_zone = stonehearth.mining:dig_region(player_id, world_region)
   if not mining_zone then
      return
   end

   local id = mining_zone:get_id()
   if not self._mining_zones[id] then
      self._mining_zones[id] = mining_zone

      mining_zone:add_component('stonehearth:mining_zone')
                     :set_selectable(false)

      local trace = mining_zone:get_component('destination')
                                    :trace_region('fabricator mining trace', TraceCategories.SYNC_TRACE)
                                       :on_changed(function(region)
                                             self:_update_dst_region()
                                          end)

      self._mining_traces[id] = trace
      trace:push_object_state()

      -- Needs to be pre_destroy!  Otherwise, the mining region is destroyed, which triggers
      -- the callback in the fabricator to update the region, which accessess the cached
      -- destination component, which blows up.
      radiant.events.listen_once(mining_zone, 'radiant:entity:pre_destroy', function()
         self._mining_zones[id] = nil
         if self._mining_traces[id] then
            self._mining_traces[id]:destroy()
            self._mining_traces[id] = nil
         end
      end)
   end
end

-- Called when the blueprint changes; calculate the intersection of the entire blueprint with the world
-- and set that to the 'total_mining_zone' (used primarily on the client for cutting away regions of 
-- the terrain in the renderer).  Note that we don't ever listen to the mining zones for recalculation; 
-- this is arguably wrong, since the mining zone will shrink as it is mined.  However, we don't see any 
-- visual artifacting (right now!), and only updating on blueprint change is potentially a lot faster.
function Fabricator:_update_total_mining_region()
   if not self._blueprint_dst:is_valid() then
      -- everything is being destroyed
      return
   end

   self._total_mining_region:modify(function(cursor)
         local world_region = radiant.entities.local_to_world(self._blueprint_dst:get_region():get(), self._entity)
         world_region = radiant.terrain.intersect_region(world_region)
         if world_region:empty() then
            cursor:clear()
            return
         end
         cursor:copy_region(world_region)
      end)
end

-- the region of the fabricator is defined as the blueprint region
-- minus the project region.  this represents the totality of the
-- finished project minus the part which is already done.  in other
-- words, the part of the project which is yet to be built (ie. the
-- part that needs to be fabricated!).  make sure this is always so,
-- regardless of how those regions change
function Fabricator:_update_fabricator_region()
   local br = self._teardown and emptyRegion or self._blueprint_dst:get_region():get()
   local pr = self._project_dst:get_region():get()

   -- rgn(f) = rgn(b) - rgn(p) ... (see comment above)
   local teardown_region = pr - br
   self._log:spam('blueprint region: %s   area:%d', br:get_bounds(), br:get_area())
   self._log:spam('project   region: %s   area:%d', pr:get_bounds(), br:get_area())
   self._log:spam('teardown  region: %s   area:%d', teardown_region:get_bounds(), teardown_region:get_area())

   self._should_teardown = not teardown_region:empty()
   if self._should_teardown then
      self._log:debug('tearing down the top row')
      self._fabricator_rcs:get_region():modify(function(cursor)
         -- we want to teardown from the top down, so just keep
         -- the topmost row of the teardown region
         local top = teardown_region:get_bounds().max.y - 1
         local clipper = Region3(Cube3(Point3(-COORD_MAX, -COORD_MAX, -COORD_MAX),
                                       Point3(COORD_MAX, top, COORD_MAX)))

         local top_slice = teardown_region - clipper
         local top_slice_bounds  = top_slice:get_bounds()

         -- we want to avoid orphening blocks when tearing down structions.  by 'orphen'
         -- i mean the block is unconnected to the rest of the structure (imagine eating a
         -- candy bar from the middle... not good!).  to do this, we'll remove the blocks that
         -- are "least connected" to their neighbors (e.g. the two ends of the candy bar 
         -- have a connectivity count of 1, whereas the middle has a count of 2)
         
         local points_by_connect_count = { {}, {}, {}, {}, {} } -- a table to rank points
         
         for top_slice_cube in top_slice:each_cube() do
            -- we know points in the middle of the region have a connectivity count of 4,
            -- so we're definitely not going to use those.  iterate through every point in
            -- the border to find candiates for poorly connected cubes.
            for pt in top_slice_cube:get_border():each_point() do
               local rank = 1
               -- keep a count of the number of points adjacent to this one, then add
               -- this point to the rank table. 
               for _, offset in ipairs(ADJACENT_POINTS) do
                  local test_point = pt + offset
                  if not top_slice_bounds:contains(test_point) and not top_slice:contains(test_point) then
                     rank = rank + 1
                  end
               end
               table.insert(points_by_connect_count[rank], pt)
            end
         end
         -- choose the least connected points (i.e. those points with the highest rank).
         local points_to_add = {}
         for i=#points_by_connect_count,1,-1 do
            if #points_by_connect_count[i] > 0 then
               points_to_add = points_by_connect_count[i]
               break
            end
         end
         
         -- now add all those points to the region
         cursor:clear()        
         for _, pt in ipairs(points_to_add) do
            cursor:add_unique_point(pt)
         end
         cursor:optimize_by_merge('update fab region')
         self._finished = cursor:empty()
      end)
   else
      self._log:debug('updating build region')     
      self._fabricator_rcs:get_region():modify(function(cursor)
         local rgn = br - pr
         cursor:copy_region(rgn)
         self._log:debug('still have %d voxels left to build', cursor:get_area())
         self._finished = cursor:empty()
      end)
   end

   if self._blueprint and self._blueprint:is_valid() then
      self._blueprint:add_component('stonehearth:construction_progress')
                        :set_finished(self._finished)
      -- finishing in teardown mode might cause us to immediately be destroyed.
      -- if so, there's no point in continuing.
      if not self._entity:is_valid() then
         return
      end
   end

   for cube in (br - pr):each_cube() do
      self._log:spam('  br - pr cube:   %s : %d', cube, cube.tag)
   end
   self:_log_destination(self._entity)
   self:_log_destination(self._blueprint)
   self:_log_destination(self._project)

   if self._finished then
      self:_stop_project()
   else
      self:_start_project()
   end
end

function Fabricator:_trace_blueprint_and_project()
   local update_blueprint_region = function()
      if self._active then
         self:_update_mining_region()
      end   
      self:_update_total_mining_region()
      self:_update_fabricator_region()
   end
   
   local update_fabricator_region = function()
      self:_update_fabricator_region()
      self:_update_total_mining_region()
   end
   
   local btrace = self._blueprint_dst:trace_region('updating fabricator', TraceCategories.SYNC_TRACE)
                                     :on_changed(update_blueprint_region)

   local ptrace  = self._project_dst:trace_region('updating fabricator', TraceCategories.SYNC_TRACE)
                                     :on_changed(update_fabricator_region)

   table.insert(self._traces, btrace)
   table.insert(self._traces, ptrace)
   
   update_fabricator_region()
end

function Fabricator:_log_region(r, name)
   self._log:detail('region %s (bounds:%s)', name, r:get_bounds())
   for c in r:each_cube() do
      self._log:detail('  %s', c)
   end   
end

if radiant.log.is_enabled('build.fabricator', radiant.log.SPAM) then
   function Fabricator:_log_destination(entity)
      self._log:spam('destination for %s', entity)
      local dst = entity:get_component('destination')
      local region = dst:get_region()
      if region then
         --region:modify(function(c) c:force_optimize_by_merge('debuggin') end)
         for cube in region:get():each_cube() do
            self._log:spam('  region cube:   %s : %d', cube, cube.tag)
         end
      end
      local reserved = dst:get_reserved()
      if reserved then
         --region:modify(function(c) c:force_optimize_by_merge('debuggin') end)
         for cube in reserved:get():each_cube() do
            self._log:spam('  reserved cube: %s : %d', cube, cube.tag)
         end
      end
      local adjacent = dst:get_adjacent()
      if adjacent then
         --region:modify(function(c) c:force_optimize_by_merge('debuggin') end)
         for cube in dst:get_adjacent():get():each_cube() do
            self._log:spam('  adjacent cube: %s : %d', cube, cube.tag)
         end
      end
   end
else
   function Fabricator:_log_destination(entity)
   end
end

return Fabricator

