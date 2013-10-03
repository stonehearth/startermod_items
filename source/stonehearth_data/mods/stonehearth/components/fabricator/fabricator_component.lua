local FabricatorComponent = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

-- this is the component which manages the fabricator entity.
function FabricatorComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
   self._width = 0
   self._normal = Point2(0, 1)
end

function FabricatorComponent:start_project(location, blueprint)
   self._blueprint = blueprint

   local faction = radiant.entities.get_faction(blueprint)
   local wss = radiant.mods.load('stonehearth').worker_scheduler
   self._worker_scheduler = wss:get_worker_scheduler(faction)

   -- initialize the fabricator entity.
   local rgn = _radiant.sim.alloc_region()
   self._entity:add_component('destination')
                     :set_region(rgn)
                     :set_auto_update_adjacent(true)
   
   -- create a new project.  projects start off completely unbuilt.
   rgn = _radiant.sim.alloc_region()
   self._project  = radiant.entities.create_entity(blueprint:get_uri())
   self._project:add_component('destination')
                     :set_region(rgn)
                     :set_auto_update_adjacent(true)
   radiant.entities.set_faction(self._project, radiant.entities.get_faction(blueprint))

   self:_trace_blueprint_and_project()
   self:_start_pickup_task()
   self:_start_fabricate_task()

   -- finally, put it on the ground.  we also need to put ourselves on the ground
   -- so the pathfinder can find it's way to regions which need to be constructed
   radiant.terrain.place_entity(self._project, location)
   radiant.terrain.place_entity(self._entity, location)
end

function FabricatorComponent:add_block(material_entity, location)
   -- location is in world coordinates.  transform it to the local coordinate space
   -- before building
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local cursor = self._project:add_component('destination'):get_region():modify()
   cursor:add_point(location - origin)
end

function FabricatorComponent:destroy()
   self:_untrace_blueprint_and_project()
   self:_stop_worker_task()
end

function FabricatorComponent:_start_pickup_task()  
   local worker_filter_fn = function(worker)
      return not radiant.entities.get_carrying(worker)
   end
   
   local work_obj_filter_fn = function(entity)
      return entity and entity:get_component('item') ~= nil
   end   

   self._pickup_task = self._worker_scheduler:add_worker_task('pickup to fabricate')
                           :set_action('stonehearth.pickup_item_on_path')
                           :set_worker_filter_fn(worker_filter_fn)
                           :set_work_object_filter_fn(work_obj_filter_fn)
                           :start()
end

function FabricatorComponent:_start_fabricate_task()                           
   local worker_filter_fn = function(worker)
      local carrying = radiant.entities.get_carrying(worker)
      return carrying ~= nil
   end
   
   self._fabricate_task = self._worker_scheduler:add_worker_task('fabricate!')
                           :set_worker_filter_fn(worker_filter_fn)
                           :add_work_object(self._entity)
                           :set_action('stonehearth.fabricate')
                           :start()
end

function FabricatorComponent:_stop_worker_task()
   if self._worker_task then
      self._worker_task:stop()
    end
end

function FabricatorComponent:_untrace_blueprint_and_project()
   if self._blueprint_region_trace then
      self._blueprint_region_trace:destroy()
      self._project_region_trace = nil
   end
   if self._project_region_trace then
      self._project_region_trace:destroy()
      self._project_region_trace = nil
   end
end

function FabricatorComponent:_trace_blueprint_and_project()
   -- the region of the fabricator is defined as the bblueprint region
   -- minus the project region.  this represents the totality of the
   -- finished project minus the part which is already done.  in other
   -- words, the part of the project which is yet to be built (ie. the
   -- part that needs to be fabricated!).  make sure this is always so,
   -- regardless of how those regions change
   local update_fabricator_region = function()
      local br = self._blueprint:add_component('destination'):get_region():get()
      local pr = self._project:add_component('destination'):get_region():get()

      local cursor = self._entity:add_component('destination'):get_region():modify()
      cursor:copy_region(br)
      cursor:subtract_region(pr)
      radiant.log.warning('(xxx) setting fabricator %s region %d to %s', tostring(self._entity), self._entity:add_component('destination'):get_region():get_id(), tostring(cursor))
   end
     
   self._blueprint_region_trace = self._blueprint:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)

   self._project_region_trace = self._project:add_component('destination'):trace_region('updating fabricator')
                                       :on_changed(update_fabricator_region)
   update_fabricator_region()
end

return FabricatorComponent

