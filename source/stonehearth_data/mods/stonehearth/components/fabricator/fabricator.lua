local priorities = require('constants').priorities.worker_task
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3

local Fabricator = class()
local log = radiant.log.create_logger('build.fabricator')

local COORD_MAX = 1000000 -- 1 million enough?

-- this is the component which manages the fabricator entity.
function Fabricator:__init(name, entity, blueprint)
   self.name = name
   log:debug('%s creating fabricator', self.name)
   
   self._entity = entity
   self._blueprint = blueprint
   self._teardown = false
   self._faction = radiant.entities.get_faction(blueprint)

   self._resource_material = blueprint:get_component('stonehearth:construction_data'):get_material()
   
   -- initialize the fabricator entity.  we'll manually update the
   -- adjacent region of the destination so we can build the project
   -- in layers.  this helps prevent the worker from getting stuck
   -- and just looks cooler
   local dst = self._entity:add_component('destination')
   dst:set_region(_radiant.sim.alloc_region())
      :set_reserved(_radiant.sim.alloc_region())
      :set_adjacent(_radiant.sim.alloc_region())
       
   log:debug("%s fabricator tracing destination %d", self.name, dst:get_id())
   self._traces = {}
   local update_adjacent = function()
      self:_update_adjacent()
   end
   table.insert(self._traces, dst:trace_region('fabricator adjacent'):on_changed(update_adjacent))
   table.insert(self._traces, dst:trace_reserved('fabricator adjacent'):on_changed(update_adjacent))
   
   self._construction_data = blueprint:get_component('stonehearth:construction_data')
   assert(self._construction_data)
                     
   -- create a new project.  projects start off completely unbuilt.
   -- projects are stored in as children to the fabricator, so there's
   -- no need to update their transform.
   local rgn = _radiant.sim.alloc_region()
   self._project = radiant.entities.create_entity(blueprint:get_uri())
   self._project:add_component('destination')
                     :set_region(rgn)
                     
   self._project:add_component('region_collision_shape')
                     :set_region(rgn)
   radiant.entities.set_faction(self._project, blueprint)
   self._entity:add_component('entity_container')
                     :add_child(self._project)

   -- get fabrication specific info, if available.  copy it into the project, too
   -- so everything gets rendered correctly.
   local state = self._construction_data:get_savestate()
   self._project:add_component('stonehearth:construction_data', state)

   -- hold onto the blueprint ladder component, if it exists.  we'll replicate
   -- the ladder into the project as it gets built up.
   self._blueprint_ladder = blueprint:get_component('vertical_pathing_region')
   if self._blueprint_ladder then
      self._project_ladder = self._project:add_component('vertical_pathing_region')
      self._project_ladder:set_region(_radiant.sim.alloc_region())
   end
   
   local progress = self._project:get_component('stonehearth:construction_progress')
   if progress then
      self._dependencies_finished = progress:check_dependencies()
      radiant.events.listen(self._project, 'stonehearth:construction_dependencies_finished', self, self._on_dependencies_finished)
   else
      self._dependencies_finished = true
   end
   self:_trace_blueprint_and_project()
end

function Fabricator:_on_dependencies_finished(e)
   self._dependencies_finished = e.finished
   if e.finished then
      self:_start_project()
   else
      self:_stop_project()
   end
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

   self._project:add_component('destination'):get_region():modify(function(cursor)
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
   if self._teardown then
      return nil
   end
   
   if carrying then
      local origin = radiant.entities.get_world_grid_location(self._entity)
      local pt = location - origin
      
      local dst = self._entity:add_component('destination')
      local adjacent = dst:get_adjacent():get()
      if adjacent:contains(pt) then
         local block = dst:get_point_of_interest(pt) + origin
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

   local rgn = self._entity:get_component('destination'):get_region():get()
   if not rgn:contains(pt) then
      log:warning('%s trying to remove unbuild portion %s of construction project', self.name, pt)
      return false
   end
   
   self._project:add_component('destination'):get_region():modify(function(cursor)
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

function Fabricator:destroy()
   for _, trace in ipairs(self._traces) do
      trace:destroy()
   end
   self._traces = {}
   self:_untrace_blueprint_and_project()
   self:_stop_project()
end

function Fabricator:_start_project()
   local run_teardown_task, run_fabricate_task

   -- If we're tearing down the project, we only need to start the teardown
   -- task.  If we're building up and all our dependencies are finished
   -- building up, start the pickup and fabricate tasks
   if self._teardown then
      run_teardown_task = true
   elseif self._dependencies_finished then
      run_fabricate_task = true
   end
   
   -- Now apply the deltas.  Create tasks that need creating and destroy
   -- ones that need destroying.
   if run_teardown_task and not self._teardown_task then
      self:_start_teardown_task()
   elseif not run_teardown_task and self._teardown_task then
      self._teardown_task:destroy()
      self._teardown_task = nil
      radiant.events.trigger(self, 'needs_teardown', self, false)
   end
   
   if run_fabricate_task and not self._fabricate_task then
      self:_start_fabricate_task()
   elseif not run_fabricate_task and self._fabricate_task then
      self._fabricate_task:destroy()
      self._fabricate_task = nil
      radiant.events.trigger(self, 'needs_work', self, false)
   end
end

function Fabricator:_start_teardown_task()
   assert(not self._teardown_task)
   
   local town = stonehearth.town:get_town(self._entity)
   self._teardown_task = town:create_worker_task('stonehearth:teardown_structure', { fabricator = self })
                                   :set_name('teardown')
                                   :set_source(self._entity)
                                   :set_max_workers(self:get_max_workers())
                                   :set_priority(priorities.TEARDOWN_BUILDING)
                                   :start()
   radiant.events.trigger(self, 'needs_teardown', self, true)
end
 
function Fabricator:_start_fabricate_task() 
   assert(not self._fabricate_task)

   local town = stonehearth.town:get_town(self._entity)
   self._fabricate_task = town:create_worker_task('stonehearth:fabricate_structure', { fabricator = self })
                                   :set_name('fabricate')
                                   :set_source(self._entity)
                                   :set_max_workers(self:get_max_workers())
                                   :set_priority(priorities.CONSTRUCT_BUILDING)
                                   :start()
   radiant.events.trigger(self, 'needs_work', self, true)
end

function Fabricator:needs_work()
   return self._fabricate_task ~= nil
end

function Fabricator:needs_teardown()
   return self._teardown_task ~= nil
end

function Fabricator:reserve_block(location)
   local dst = self._entity:add_component('destination')
   local pt = location - radiant.entities.get_world_grid_location(self._entity)
   log:debug('adding point %s to reserve region', tostring(pt))
   dst:get_reserved():modify(function(cursor)
      cursor:add_point(pt)
   end)
   log:debug('finished adding point %s to reserve region', tostring(pt))
end

function Fabricator:release_block(location)
   local dst = self._entity:add_component('destination')
   local pt = location - radiant.entities.get_world_grid_location(self._entity)
   log:debug('removing point %s from reserve region', tostring(pt))
   dst:get_reserved():modify(function(cursor)
      cursor:subtract_point(pt)
   end)
   log:debug('finished removing point %s from reserve region', tostring(pt))
end

function Fabricator:get_max_workers()
   return self._construction_data:get_max_workers()
end

function Fabricator:_stop_project()
   log:warning('%s fabricator stopping all worker tasks', self.name)
   if self._teardown_task then
      self._teardown_task:destroy()
      self._teardown_task = nil
      radiant.events.trigger(self, 'needs_teardown', self, false)
   end
   if self._fabricate_task then
      self._fabricate_task:destroy()
      self._fabricate_task = nil
      radiant.events.trigger(self, 'needs_work', self, false)
   end
end

function Fabricator:_untrace_blueprint_and_project()
   if self._blueprint_region_trace then
      self._blueprint_region_trace:destroy()
      self._project_region_trace = nil
   end
   if self._project_region_trace then
      self._project_region_trace:destroy()
      self._project_region_trace = nil
   end
end

function Fabricator:_update_adjacent()
   local dst = self._entity:add_component('destination')
   local dst_rgn = dst:get_region():get()
   local reserved_rgn = dst:get_reserved():get()
   
   -- build in layers.  stencil out all but the bottom layer of the 
   -- project.  work against the destination region (not rgn - reserved).
   -- otherwise, we'll end up moving to the next row as soon as the last
   -- block in the current row is reserved.
   local bottom = dst_rgn:get_bounds().min.y
   local clipper = Region3(Cube3(Point3(-COORD_MAX, bottom + 1, -COORD_MAX),
                                 Point3( COORD_MAX, COORD_MAX,   COORD_MAX)))
   local bottom_row = dst_rgn - clipper
   
   local allow_diagonals = self._construction_data:get_allow_diagonal_adjacency()
   local adjacent = bottom_row:get_adjacent(allow_diagonals, 0, 0)

   -- some projects want the worker to stand at the base of the project and
   -- push columns up.  for example, scaffolding always gets built from the
   -- base.  if this is one of those, translate the adjacent region all the
   -- way to the bottom.
   if self._construction_data:get_project_adjacent_to_base() then
      adjacent:translate(Point3(0, -bottom, 0))
   end
   
   -- remove parts of the adjacent region which overlap the fabrication region.
   -- otherwise we get weird behavior where one worker can build a block right
   -- on top of where another is standing to build another block, or workers
   -- can build blocks to hoist themselves up to otherwise unreachable spaces,
   -- getting stuck in the process
   adjacent:subtract_region(dst_rgn)
   
   -- now subtract out all the blocks that we've reserved
   adjacent:subtract_region(reserved_rgn)
   
   -- finally, copy into the adjacent region for our destination
   dst:get_adjacent():modify(function(cursor)
      cursor:copy_region(adjacent)
   end)
end

function Fabricator:_trace_blueprint_and_project()
   -- the region of the fabricator is defined as the blueprint region
   -- minus the project region.  this represents the totality of the
   -- finished project minus the part which is already done.  in other
   -- words, the part of the project which is yet to be built (ie. the
   -- part that needs to be fabricated!).  make sure this is always so,
   -- regardless of how those regions change
   local update_fabricator_region = function()
      local br = self._blueprint:get_component('destination'):get_region():get()
      local pr = self._project:get_component('destination'):get_region():get()

      -- rgn(f) = rgn(b) - rgn(p) ... (see comment above)
      local dst = self._entity:add_component('destination')
      log:debug("%s fabricator modifying dst %d", self.name, dst:get_id())

      local teardown_region = pr - br
      local finished = false
      self._teardown = not teardown_region:empty()

      if self._teardown then
         dst:get_region():modify(function(cursor)
            -- we want to teardown from the top down, so just keep
            -- the topmost row of the teardown region
            local top = teardown_region:get_bounds().max.y - 1
            local clipper = Region3(Cube3(Point3(-COORD_MAX, -COORD_MAX, -COORD_MAX),
                                          Point3(COORD_MAX, top, COORD_MAX)))
            cursor:copy_region(teardown_region - clipper)
            finished = false
         end)
      else
         dst:get_region():modify(function(cursor)
            cursor:copy_region(br - pr)
            finished = cursor:empty()
         end)
      end

      self._project:add_component('stonehearth:construction_progress')
                     :set_finished(finished)
      if finished then
         self:_stop_project()
      else
         self:_start_project()
      end
      log:debug('%s updating region -> %s', self.name, dst:get_region():get())
   end
     
   self._blueprint_region_trace = self._blueprint:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)

   self._project_region_trace = self._project:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)
   update_fabricator_region()
end

return Fabricator

