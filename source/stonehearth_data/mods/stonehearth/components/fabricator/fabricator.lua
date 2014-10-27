local build_util = require 'lib.build_util'
local priorities = require('constants').priorities.simple_labor
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
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

-- this is the component which manages the fabricator entity.
function Fabricator:__init(name, entity, blueprint, project)
   self.name = name

   self._log = radiant.log.create_logger('build')
                        :set_prefix('fab for ' .. tostring(blueprint))
                        :set_entity(entity)

   self._log:debug('creating fabricator')
   
   self._finished = false
   self._entity = entity
   self._fabricator_dst = self._entity:add_component('destination')
   self._fabricator_rcs = self._entity:add_component('region_collision_shape')
   self._blueprint = blueprint
   self._blueprint_dst = blueprint:get_component('destination')
   self._blueprint_ladder = blueprint:get_component('vertical_pathing_region')   
   self._blueprint_construction_data = blueprint:get_component('stonehearth:construction_data')
   self._blueprint_construction_progress = blueprint:get_component('stonehearth:construction_progress')
   self._mining_zones = {}
   self._mining_traces = {}

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
   
   if self._blueprint_construction_progress then
      self._dependencies_finished = self._blueprint_construction_progress:check_dependencies()
      self._finished_listener = radiant.events.listen(self._blueprint, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)
   else
      self._dependencies_finished = true
   end   
   self:_trace_blueprint_and_project()
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

function Fabricator:set_active(active)
   self._active = active
   if self._active then
      self:_start_project()
   else
      self:_stop_project()
   end
end

function Fabricator:set_teardown(teardown)
   self._teardown = teardown
   self:_update_fabricator_region()
end

function Fabricator:_initialize_existing_project(project)  
   self._project = project
   self._project_dst = self._project:get_component('destination')
   self._project_ladder = self._project:get_component('vertical_pathing_region')

   self._fabricator_dst:get_reserved():modify(function(cursor)
      cursor:clear()
   end)
end

function Fabricator:instabuild()
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
   self._project = radiant.entities.create_entity(blueprint:get_uri())
   self._project_dst = self._project:add_component('destination')

   radiant.entities.set_faction(self._project, blueprint)
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

   -- we'll replicate the ladder into the project as it gets built up.
   if self._blueprint_ladder then
      self._project_ladder = self._project:add_component('vertical_pathing_region')
      self._project_ladder:set_region(_radiant.sim.alloc_region3())
   end   
end

function Fabricator:_on_dependencies_finished_changed()
   self._dependencies_finished = self._blueprint_construction_progress
                                                :get_dependencies_finished()
   self._log:debug('got stonehearth:construction:dependencies_finished_changed event (dependencies finished = %s)',
                   tostring(self._dependencies_finished))

   self:_start_project()
end

function Fabricator:get_material()
   return self._resource_material
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

function Fabricator:add_block(material_entity, location)  
   if not self._entity:is_valid() then
      return false
   end
   
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building  
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

   -- if we've projected the fabricator region to the base of the project,
   -- the location passed in will be at the base, too.  find the appropriate
   -- block to add to the project collision shape by starting at the bottom
   -- and looking up.
   if self._blueprint_construction_data:get_project_adjacent_to_base() then
      local project_rgn = self._project_dst:get_region():get()
      while project_rgn:contains(pt) do
         pt.y = pt.y + 1
      end
      if not self._blueprint_dst:get_region():get():contains(pt) then
         -- we couldn't find a block in this column that is both missing from the
         -- project and inside the blueprint.  we must already be done!!
         self._log:info('skipping location %s -> %s.  no longer in blueprint!', location, pt + origin)
         return
      end
   end

   self._project_dst:get_region():modify(function(cursor)
         cursor:add_point(pt)
      end)
   self:release_block(location)
   
   -- ladders are a special case used for scaffolding.  if there's one on the
   -- blueprint at this location, go ahead and add it to the project as well.
   if self._blueprint_ladder then
      local rgn = self._blueprint_ladder:get_region():get()
      local normal = self._blueprint_ladder:get_normal()
      if rgn:contains(pt + normal) then
         self._project_ladder:get_region():modify(function(cursor)
            cursor:add_point(pt + normal)
         end)
      end   
   end
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
         local block = self._fabricator_dst:get_point_of_interest(pt) + origin

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

function Fabricator:remove_block(location)
   if not self._entity:is_valid() then
      return false
   end
   
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

   local rgn = self._fabricator_dst:get_region():get()
   if not rgn:contains(pt) then
      self._log:warning('trying to remove unbuild portion %s of construction project', pt)
      return false
   end

   -- if we've projected the fabricator region to the base of the project,
   -- the location passed in will be at the base, too.  find the appropriate
   -- block to remove to the project collision shape by starting at the tip
   -- top and looking down
   if self._blueprint_construction_data:get_project_adjacent_to_base() then
      local project_rgn = self._project_dst:get_region():get()
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
   if self._project_ladder then
      local rgn = self._project_ladder:get_region()
      local normal = self._blueprint_ladder:get_normal()
      if rgn:get():contains(pt + normal) then
         rgn:modify(function(cursor)
            cursor:subtract_point(pt + normal)
         end)
      end   
   end
   return true
end

function Fabricator:_start_project()
   local run_teardown_task, run_fabricate_task

   -- If we're tearing down the project, we only need to start the teardown
   -- task.  If we're building up and all our dependencies are finished
   -- building up, start the pickup and fabricate tasks
   self._log:detail('start_project (activated:%s teardown:%s finished:%s deps_finished:%s)', tostring(self._active), tostring(self._should_teardown), self._finished, self._dependencies_finished)
   if self._finished then
      return
   end

   if self._active and self._dependencies_finished then
      if self._should_teardown then
         run_teardown_task = true
      else
         run_fabricate_task = true
      end
   end
   
   -- Now apply the deltas.  Create tasks that need creating and destroy
   -- ones that need destroying.
   if run_teardown_task then
      self:_start_teardown_task()
   else
      self:_destroy_teardown_task()
   end
   
   if run_fabricate_task then
      self:_start_fabricate_task()
   else
      self:_destroy_fabricate_task()
   end
end

function Fabricator:_start_teardown_task()
   if not self._teardown_task then
      self._log:debug('starting teardown task')
      local town = stonehearth.town:get_town(self._project)
      self._teardown_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:teardown_structure', { fabricator = self })
                                      :set_name('teardown')
                                      :set_source(self._entity)
                                      :set_max_workers(self:get_max_workers())
                                      :set_priority(priorities.TEARDOWN_BUILDING)
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
   self:_update_mining_region()
   if not self._fabricate_task then
      self._log:debug('starting fabricate task')
      local town = stonehearth.town:get_town(self._blueprint)
      self._fabricate_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:fabricate_structure', { fabricator = self })
                                      :set_name('fabricate')
                                      :set_source(self._entity)
                                      :set_max_workers(self:get_max_workers())
                                      :set_priority(priorities.CONSTRUCT_BUILDING)
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
      for pt in dst_region:each_point() do
         pt.y = 0
         while not shape_region:contains(pt) do
            pt.y = pt.y + 1
         end
         dr:add_unique_point(pt)
      end
      dst_region = dr
   end
   dst_region:optimize_by_merge()

   --self._log:detail('update dst region')
   --self:_log_region(rcs_rgn, 'region collision shape ->')
   --self:_log_region(dst_region, 'resulted in destination ->')
   
   -- Any region that needs mining should be removed from our destination region.
   for zone,_ in pairs(self._mining_zones) do
      if zone:get_component('destination') then
         local mining_region = zone:get_component('destination'):get_region():get()

         dst_region = dst_region - mining_region:translated(Point3(0, -1, 0))
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


   -- if there's a normal, stencil off the adjacent blocks pointing in
   -- in the opposite direction.  this is to stop people from working on walls
   -- from the inside of the building (lest they be trapped!)
   local normal = self._blueprint_construction_data:get_normal()
   if normal then
      adjacent:subtract_region(dst_rgn:translated(-normal))
   end

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

-- Called when the blueprint region is changed (added or merged).  Not called when the
-- fabricator is told to stop.
function Fabricator:_update_mining_region()
   local player_id = radiant.entities.get_player_id(self._blueprint)
   local faction = radiant.entities.get_faction(self._blueprint)

   -- The mining service will handle all overlap merging for us.
   local world_pos = radiant.entities.get_parent(self._entity):get_component('mob'):get_location()
   local world_region = self._blueprint_dst:get_region():get():translated(world_pos)
   local mining_zone = stonehearth.mining:dig_region(player_id, faction, world_region)
   
   if not self._mining_zones[mining_zone] then
      self._mining_zones[mining_zone] = true

      local mining_trace = mining_zone:get_component('destination'):trace_region('fabricator mining trace', TraceCategories.SYNC_TRACE)
         :on_changed(function(region)
            self:_update_dst_region()
         end)
         :push_object_state()

      table.insert(self._mining_traces, mining_trace)
   end
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
         self._finished = cursor:empty()
      end)
   else
      self._log:debug('updating build region')     
      self._fabricator_rcs:get_region():modify(function(cursor)
         local rgn = br - pr
         cursor:copy_region(rgn)
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
   
   if self._finished then
      self:_stop_project()
   else
      self:_start_project()
   end
end

function Fabricator:_trace_blueprint_and_project()
   local update_fabricator_region = function()
      if self._active then
         self:_update_mining_region()
      end
      self:_update_fabricator_region()
   end
   
   local btrace = self._blueprint_dst:trace_region('updating fabricator', TraceCategories.SYNC_TRACE)
                                     :on_changed(update_fabricator_region)

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

return Fabricator

