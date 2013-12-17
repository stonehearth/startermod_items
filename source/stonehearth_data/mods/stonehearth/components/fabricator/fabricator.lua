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
   self._entity = entity
   self._blueprint = blueprint
   self._teardown = false
   
   local faction = radiant.entities.get_faction(blueprint)
   local wss = radiant.mods.load('stonehearth').worker_scheduler
   self._worker_scheduler = wss:get_worker_scheduler(faction)

   -- initialize the fabricator entity.  we'll manually update the
   -- adjacent region of the destination so we can build the project
   -- in layers.  this helps prevent the worker from getting stuck
   -- and just looks cooler
   local dst = self._entity:add_component('destination')
   dst:set_region(_radiant.sim.alloc_region())
      :set_adjacent(_radiant.sim.alloc_region())
       
   log:debug("fabricator tracing destination %d", dst:get_id())
   self._adjacent_guard = dst:trace_region('updating fabricator adjacent')
   self._adjacent_guard:on_changed(function ()
      self:_update_adjacent()
   end)
   
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
   self._project:add_component('stonehearth:construction_data')
                  :extend(self._construction_data:get_data()) -- actually 'load' or something.
      
   -- hold onto the blueprint ladder component, if it exists.  we'll replicate
   -- the ladder into the project as it gets built up.
   self._blueprint_ladder = blueprint:get_component('vertical_pathing_region')
   if self._blueprint_ladder then
      self._project_ladder = self._project:add_component('vertical_pathing_region')
      self._project_ladder:set_region(_radiant.sim.alloc_region())
   end
   
   self:_trace_blueprint_and_project()
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
   assert(not self._teardown)
   
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

   self._project:add_component('destination'):get_region():modify(function(cursor)
      cursor:add_point(pt)
   end)
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

function Fabricator:remove_block(location)
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local pt = location - origin

   local rgn = self._entity:get_component('destination'):get_region():get()
   if not rgn:contains(pt) then
      log:warning('trying to remove unbuild portion %s of construction project', pt)
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
   if self._adjacent_guard then
      self._adjacent_guard:destroy()
      self._adjacent_guard = nil
   end
   self:_untrace_blueprint_and_project()
   self:_stop_project()
end

function Fabricator:set_debug_color(color)
   if self._pickup_task then
      self._pickup_task:set_debug_color(color)
   end
   if self._teardown_task then
      self._teardown_task:set_debug_color(color)
   end
   if self._fabricate_task then
      self._fabricate_task:set_debug_color(color)
   end
   return self
end

function Fabricator:_start_project()
   self._project:add_component('stonehearth:construction_data')
                  :set_finished(false)

   if self._teardown then
      if self._pickup_task then
         self._pickup_task:stop()
         self._pickup_task = nil
      end
      if self._fabricate_task then
         self._fabricate_task:stop()
         self._fabricate_task = nil
      end
      if not self._teardown_task then
         self:_start_teardown_task()
      end
   else
      if self._teardown_task then
         self._teardown_task:stop()
         self._teardown_task = nil
      end
      if not self._pickup_task then
         self:_start_pickup_task()
      end
      if not self._fabricate_task then
         self:_start_fabricate_task()
      end
   end
end

function Fabricator:_start_teardown_task()
   local worker_filter_fn = function(worker)
      local carrying  = radiant.entities.get_carrying(worker)
      if not carrying then
         return true
      end
      if not radiant.entities.is_material(carrying, 'wood resource') then
         return false
      end
      local item = carrying:get_component('item')
      if not item then
         return false
      end
      return item:get_stacks() < item:get_max_stacks()
   end
   
   local name = 'teardown ' .. self.name
   self._teardown_task = self._worker_scheduler:add_worker_task(name)
                           :set_worker_filter_fn(worker_filter_fn)
                           :add_work_object(self._entity)
                           :set_action('stonehearth:teardown')
                           :set_priority(priorities.TEARDOWN_BUILDING)
                           :start()
end
 
function Fabricator:_start_pickup_task()  
   local worker_filter_fn = function(worker)
      return not radiant.entities.get_carrying(worker)
   end
   
   local work_obj_filter_fn = function(item)
      return radiant.entities.is_material(item, 'wood resource')
   end   

   local name = 'pickup to fabricate ' .. self.name
   self._pickup_task = self._worker_scheduler:add_worker_task(name)
                           :set_action('stonehearth:pickup_item_on_path')
                           :set_worker_filter_fn(worker_filter_fn)
                           :set_work_object_filter_fn(work_obj_filter_fn)
                           :set_priority(priorities.CONSTRUCT_BUILDING)
                           :start()
end

function Fabricator:_start_fabricate_task()                           
   local worker_filter_fn = function(worker)
      local carrying = radiant.entities.get_carrying(worker)
      return carrying ~= nil
   end
   
   local name = 'fabricate ' .. self.name
   self._fabricate_task = self._worker_scheduler:add_worker_task(name)
                           :set_worker_filter_fn(worker_filter_fn)
                           :add_work_object(self._entity)
                           :set_action('stonehearth:fabricate')
                           :set_priority(priorities.CONSTRUCT_BUILDING)
                           :start()
end

function Fabricator:_stop_project()
   log:warning('fabricator %s stopping all worker tasks', self.name)
   if self._pickup_task then
      self._pickup_task:stop()
      self._pickup_task = nil
   end
   if self._fabricate_task then
      self._fabricate_task:stop()
      self._fabricate_task = nil
   end
   self._project:add_component('stonehearth:construction_data')
                  :set_finished(true)   
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
   local rgn = dst:get_region():get()
   
   -- build in layers.  stencil out all but the bottom layer of the 
   -- project
   local bottom = rgn:get_bounds().min.y
   local clipper = Region3(Cube3(Point3(-COORD_MAX, bottom + 1, -COORD_MAX),
                                 Point3( COORD_MAX, COORD_MAX,   COORD_MAX)))
   local bottom_row = rgn - clipper
   
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
   adjacent:subtract_region(rgn)
   
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
      log:debug("fabricator modifying dst %d", dst:get_id())

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
      if finished then
         self:_stop_project()
      else
         self:_start_project()
      end
      log:debug('updating fabricator %s region -> %s', self.name, dst:get_region():get())
   end
     
   self._blueprint_region_trace = self._blueprint:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)

   self._project_region_trace = self._project:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)
   update_fabricator_region()
end

return Fabricator

