local Fabricator = class()
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3

local COORD_MAX = 1000000 -- 1 million enough?

-- this is the component which manages the fabricator entity.
function Fabricator:__init(name, entity, blueprint)
   self.name = name
   self._entity = entity
   self._blueprint = blueprint

   local entity_data = radiant.entities.get_entity_data(blueprint, 'stonehearth:fabricator')
   if entity_data then
      self._project_adjacent_to_base = entity_data.project_adjacent_to_base
   end
   
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
       
   self._adjacent_guard = dst:trace_region('updating fabricator adjacent')
   self._adjacent_guard:on_changed(function ()
      self:_update_adjacent()
   end)
   
                     
   -- create a new project.  projects start off completely unbuilt.
   local rgn = _radiant.sim.alloc_region()
   self._project  = radiant.entities.create_entity(blueprint:get_uri())      
   self._project:add_component('destination')
                     :set_region(rgn)
   self._project:add_component('region_collision_shape')
                     :set_region(rgn)
   radiant.entities.set_faction(self._project, blueprint)

   -- hold onto the blueprint ladder component, if it exists.  we'll replicate
   -- the ladder into the project as it gets built up.
   self._blueprint_ladder = blueprint:get_component('vertical_pathing_region')
   if self._blueprint_ladder then
      self._project_ladder = self._project:add_component('vertical_pathing_region')
      self._project_ladder:set_region(_radiant.sim.alloc_region())
   end
   
   self:_trace_blueprint_and_project()
   self:_start_pickup_task()
   self:_start_fabricate_task()
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
   local cursor = self._project:add_component('destination'):get_region():modify()
   local pt = location - origin
   cursor:add_point(pt)
   
   -- ladders are a special case used for scaffolding.  if there's one on the
   -- blueprint at this location, go ahead and add it to the project as well.
   if self._blueprint_ladder then
      local rgn = self._blueprint_ladder:get_region():get()
      local normal = self._blueprint_ladder:get_normal()
      if rgn:contains(pt + normal) then
         local cursor = self._project_ladder:get_region():modify()
         cursor:add_point(pt + normal)
      end
   end
end

function Fabricator:destroy()
   if self._adjacent_guard then
      self._adjacent_guard:destroy()
      self._adjacent_guard = nil
   end
   self:_untrace_blueprint_and_project()
   self:_stop_worker_tasks()
end

function Fabricator:set_debug_color(color)
   if self._pickup_task then
      self._pickup_task:set_debug_color(color)
    end
   if self._fabricate_task then
      self._fabricate_task:set_debug_color(color)
    end
    return self
end

function Fabricator:_start_pickup_task()  
   local worker_filter_fn = function(worker)
      return not radiant.entities.get_carrying(worker)
   end
   
   local work_obj_filter_fn = function(entity)
      return entity and entity:get_component('item') ~= nil
   end   

   local name = 'pickup to fabricate ' .. self.name
   self._pickup_task = self._worker_scheduler:add_worker_task(name)
                           :set_action('stonehearth.pickup_item_on_path')
                           :set_worker_filter_fn(worker_filter_fn)
                           :set_work_object_filter_fn(work_obj_filter_fn)
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
                           :set_action('stonehearth.fabricate')
                           :start()
end

function Fabricator:_stop_worker_tasks()
   if self._pickup_task then
      self._pickup_task:stop()
      self._pickup_task = nil
   end
   if self._fabricate_task then
      self._fabricate_task:stop()
      self._fabricate_task = nil
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
   local rgn = dst:get_region():get()
   
   -- build in layers.  stencil out all but the bottom layer of the 
   -- project
   local bottom = rgn:get_bounds().min.y
   local clipper = Region3(Cube3(Point3(-COORD_MAX, bottom + 1, -COORD_MAX),
                                 Point3( COORD_MAX, COORD_MAX,   COORD_MAX)))
   local bottom_row = rgn - clipper
   local adjacent = bottom_row:get_adjacent()
   
   if self._project_adjacent_to_base then
      adjacent:translate(Point3(0, -bottom, 0)) -- xxx: test this nonsense!
   end
   dst:get_adjacent():modify():copy_region(adjacent)
end

function Fabricator:_trace_blueprint_and_project()
   -- the region of the fabricator is defined as the blueprint region
   -- minus the project region.  this represents the totality of the
   -- finished project minus the part which is already done.  in other
   -- words, the part of the project which is yet to be built (ie. the
   -- part that needs to be fabricated!).  make sure this is always so,
   -- regardless of how those regions change
   local update_fabricator_region = function()
      local br = self._blueprint:add_component('destination'):get_region():get()
      local pr = self._project:add_component('destination'):get_region():get()

      -- rgn(f) = rgn(b) - rgn(p) ... (see comment above)
      local dst = self._entity:add_component('destination')
      local cursor = dst:get_region():modify()
      cursor:copy_region(br)
      cursor:subtract_region(pr)
      radiant.log.info('updating fabricator %s region -> %s', self.name, cursor)
   end
     
   self._blueprint_region_trace = self._blueprint:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)

   self._project_region_trace = self._project:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)
   update_fabricator_region()
end

return Fabricator

