local priorities = require('constants').priorities.worker_task
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3

local Fabricator = class()

local emptyRegion = Region3()
local COORD_MAX = 1000000 -- 1 million enough?
local ADJACENT_POINTS = {
   Point3(-1, 0, -1),
   Point3(-1, 0,  1),
   Point3( 1, 0, -1),
   Point3( 1, 0,  1),
}

-- this is the component which manages the fabricator entity.
function Fabricator:__init(name, entity, blueprint, project)
   self.name = name

   self._log = radiant.log.create_logger('build')
                        :set_prefix('fab for ' .. tostring(blueprint))
                        :set_entity(entity)

   self._log:debug('creating fabricator')
   
   self._entity = entity
   self._fabricator_dst = self._entity:add_component('destination')
   self._blueprint = blueprint
   self._blueprint_dst = blueprint:get_component('destination')
   self._blueprint_ladder = blueprint:get_component('vertical_pathing_region')   
   self._blueprint_construction_data = blueprint:get_component('stonehearth:construction_data')
   self._blueprint_construction_progress = blueprint:get_component('stonehearth:construction_progress')

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

   local update_adjacent = function()
      self:_update_adjacent()
   end

   table.insert(self._traces, self._fabricator_dst:trace_region('fabricator adjacent'):on_changed(update_adjacent))
   table.insert(self._traces, self._fabricator_dst:trace_reserved('fabricator adjacent'):on_changed(update_adjacent))
   
   if self._blueprint_construction_progress then
      self._dependencies_finished = self._blueprint_construction_progress:check_dependencies()
      radiant.events.listen(self._blueprint, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)
   else
      self._dependencies_finished = true
   end   
   radiant.events.listen_once(self._blueprint, 'radiant:entity:pre_destroy', self, self._on_blueprint_destroyed)
   self:_trace_blueprint_and_project()
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

function Fabricator:_create_new_project()
   -- initialize the fabricator entity.  we'll manually update the
   -- adjacent region of the destination so we can build the project
   -- in layers.  this helps prevent the worker from getting stuck
   -- and just looks cooler
   self._fabricator_dst:set_region(_radiant.sim.alloc_region())
            :set_reserved(_radiant.sim.alloc_region())
            :set_adjacent(_radiant.sim.alloc_region())
       
   -- create a new project.  projects start off completely unbuilt.
   -- projects are stored in as children to the fabricator, so there's
   -- no need to update their transform.
   local blueprint = self._blueprint
   local rgn = _radiant.sim.alloc_region()
   self._project = radiant.entities.create_entity(blueprint:get_uri())
   self._project_dst = self._project:add_component('destination')

   radiant.entities.set_faction(self._project, blueprint)
   radiant.entities.set_player_id(self._project, blueprint)

   self._project_dst:set_region(rgn)                    
   self._project:add_component('region_collision_shape')
                     :set_region(rgn)
   self._entity:add_component('entity_container')
                     :add_child(self._project)
                        
   -- get fabrication specific info, if available.  copy it into the project, too
   -- so everything gets rendered correctly.
   local state = self._blueprint_construction_data:get_savestate()
   self._project:add_component('stonehearth:construction_data', state)
                     :set_fabricator_entity(self._entity)

   -- we'll replicate the ladder into the project as it gets built up.
   if self._blueprint_ladder then
      self._project_ladder = self._project:add_component('vertical_pathing_region')
      self._project_ladder:set_region(_radiant.sim.alloc_region())
   end   
end

function Fabricator:destroy()
   self._log:debug('destroying fabricator')

   radiant.events.unlisten(self._blueprint, 'radiant:entity:pre_destroy', self, self._on_blueprint_destroyed)
   for _, trace in ipairs(self._traces) do
      trace:destroy()
   end
   self._traces = {}
   self:_stop_project()
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

function Fabricator:get_entity()
   return self._entity
end

function Fabricator:get_project()
   return self._project
end

function Fabricator:get_blueprint()
   return self._blueprint
end


function Fabricator:add_block(material_entity, location)  
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

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
         self:reserve_block(block)
         return block
      end
   end
end

function Fabricator:remove_block(location)
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

   local rgn = self._fabricator_dst:get_region():get()
   if not rgn:contains(pt) then
      self._log:warning('trying to remove unbuild portion %s of construction project', pt)
      return false
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
   self._log:detail('start_project (activated:%s teardown:%s deps_finished:%s)', self._active, self._should_teardown, self._dependencies_finished)

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
      self._teardown_task = town:create_worker_task('stonehearth:teardown_structure', { fabricator = self })
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
   if not self._fabricate_task then
      self._log:debug('starting fabricate task')
      local town = stonehearth.town:get_town(self._blueprint)
      self._fabricate_task = town:create_worker_task('stonehearth:fabricate_structure', { fabricator = self })
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

function Fabricator:_update_adjacent()
   local dst_rgn = self._fabricator_dst:get_region():get()
   local reserved_rgn = self._fabricator_dst:get_reserved():get()
   
   -- build in layers.  stencil out all but the bottom layer of the 
   -- project.  work against the destination region (not rgn - reserved).
   -- otherwise, we'll end up moving to the next row as soon as the last
   -- block in the current row is reserved.
   local bottom = dst_rgn:get_bounds().min.y
   local clipper = Region3(Cube3(Point3(-COORD_MAX, bottom + 1, -COORD_MAX),
                                 Point3( COORD_MAX, COORD_MAX,   COORD_MAX)))
   local bottom_row = dst_rgn - clipper
   
   local allow_diagonals = self._blueprint_construction_data:get_allow_diagonal_adjacency()
   local adjacent = bottom_row:get_adjacent(allow_diagonals, 0, 0)

   -- some projects want the worker to stand at the base of the project and
   -- push columns up.  for example, scaffolding always gets built from the
   -- base.  if this is one of those, translate the adjacent region all the
   -- way to the bottom.
   if self._blueprint_construction_data:get_project_adjacent_to_base() then
      adjacent:translate(Point3(0, -bottom, 0))
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
   
   -- finally, copy into the adjacent region for our destination
   self._fabricator_dst:get_adjacent():modify(function(cursor)
      cursor:copy_region(adjacent)
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
   local pr = self._project:get_component('destination'):get_region():get()

   -- rgn(f) = rgn(b) - rgn(p) ... (see comment above)
   local dst = self._entity:add_component('destination')
   self._log:debug("fabricator modifying dst %d", dst:get_id())

   local teardown_region = pr - br
   local finished = false

   self._should_teardown = not teardown_region:empty()
   if self._should_teardown then
      self._log:debug('tearing down the top row')
      dst:get_region():modify(function(cursor)
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
            for fringe_cube in top_slice_cube:get_border():each_cube() do
               for pt in fringe_cube:each_point() do
                  local rank = 1
                  -- keep a count of the number of points adjacent to this one, then add
                  -- this point to the rank table. 
                  for _, offset in ipairs(ADJACENT_POINTS) do
                     local test_point = pt + offset
                     if not top_slice_bounds:contains(test_point) and not top_slice:contains(test_point) then
                        rank = rank + 1
                     end
                  end
                  table.insert(points_by_connect_count[c], pt)
               end
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
         finished = cursor:empty()
      end)
   else
      self._log:debug('updating build region')
      dst:get_region():modify(function(cursor)
         cursor:copy_region(br - pr)
         finished = cursor:empty()
      end)
   end

   if self._blueprint and self._blueprint:is_valid() then
      self._blueprint:add_component('stonehearth:construction_progress')
                     :set_finished(finished)
   end
   
   self._log:debug('new destination region is %s', dst:get_region():get())
   if finished then
      self:_stop_project()
   else
      self:_start_project()
   end
end

function Fabricator:_trace_blueprint_and_project()
   local update_fabricator_region = function()
      self:_update_fabricator_region()
   end

   
   local dtrace = self._blueprint:trace_object('destination')
                                       :on_destroyed(update_fabricator_region)
         
   local btrace = self._blueprint_dst:trace_region('updating fabricator')
                                     :on_changed(update_fabricator_region)

   local ptrace  = self._project_dst:trace_region('updating fabricator')
                                    :on_changed(update_fabricator_region)                                       
                                       
   table.insert(self._traces, dtrace)
   table.insert(self._traces, btrace)
   table.insert(self._traces, ptrace)
   
   update_fabricator_region()
end

function Fabricator:_on_blueprint_destroyed()
   -- hm...
end

return Fabricator

