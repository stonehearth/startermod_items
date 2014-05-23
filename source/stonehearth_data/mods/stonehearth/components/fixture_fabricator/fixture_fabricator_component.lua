local priorities = require('constants').priorities.worker_task

local FixtureFabricator = class()

-- the FixtureFabricator handles the building of fixtures inside
-- structures.

-- initialize a new fixture fabricator
function FixtureFabricator:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         self:start_project()
      end)
end

-- turn the fixture fabricator off or on.  we'll only try to put the
-- fixture on the wall if the wall is finished and the fabricator is
-- active
-- 
--    @param enabled - whether or not to try to build the fixture
--
function FixtureFabricator:set_active(enabled)
   if self._sv.active ~= enabled then
      self._sv.active = enabled
      self.__saved_variables:mark_changed()

      self:_update_tasks()
   end
end

-- called to kick off the fabricator.  don't call until all the components are
-- installed and the fabricator entity is at the correct location in the wall
--
function FixtureFabricator:start_project()
   self._parent = radiant.entities.get_parent(self._entity)
   self._parent_cp = self._parent:get_component('stonehearth:construction_progress')
   radiant.events.listen(self._parent, 'stonehearth:construction:finished_changed', self, self._on_parent_finished_changed)
   self:_on_parent_finished_changed()
end

-- check to see if we should start or stop our tasks.  we'll only try to fabricate
-- when our parent is finished!
--
function FixtureFabricator:_on_parent_finished_changed()
   self._parent_finished = self._parent_cp:get_finished()
   self:_update_tasks()
end

-- helper function called by the fabricate action to actually build the fixture.
--
function FixtureFabricator:construct(worker, item)
   local project, uri

   -- xxx: for now, create a brand new entity.  eventually, we should simply be
   -- replacing the ghost_entity with the full_sized_entity for the proxy.
   local proxy_component = item:get_component('stonehearth:placeable_item_proxy')
   uri = proxy_component:get_full_sized_entity():get_uri()     
   project = radiant.entities.create_entity(uri)   

   -- take the proxy off the current worker or the terrain, depending on where
   -- it came from.
   if radiant.entities.get_carrying(worker) == item then
      radiant.entities.remove_carrying(worker)
   else
      radiant.terrain.remove_entity(item)
   end

   -- move the project to the same position as the blueprint (us)
   local location = self._entity:get_component('mob'):get_grid_location()
   radiant.entities.add_child(self._parent, project, location)
   project:add_component('mob')
               :set_rotation(self._entity:get_component('mob'):get_rotation())

   -- all done!  set a flag so we know and remember the project.
   self._sv.project = project
   self._sv.finished = true
   self.__saved_variables:mark_changed()
end

-- either create or destroy the tasks depending on our current state
--
function FixtureFabricator:_update_tasks()
   local run_teardown_task, run_fabricate_task = false, false

   if not self._sv.finished and self._sv.active and self._parent_finished then
      run_teardown_task = self._sv.should_teardown
      run_fabricate_task = not self._sv.should_teardown
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

-- start the teardown task if it's not already running
--
function FixtureFabricator:_start_teardown_task()
   if not self._teardown_task then
      local town = stonehearth.town:get_town(self._entity)
      self._teardown_task = town:create_worker_task('stonehearth:teardown_fixture', { fabricator = self._entity })
                                      :set_name('teardown')
                                      :set_source(self._entity)
                                      :set_max_workers(1)
                                      :set_priority(priorities.TEARDOWN_BUILDING)
                                      :once()
                                      :start()
   end
end

-- destroy the teardown task
--
function FixtureFabricator:_destroy_teardown_task()
   if self._teardown_task then
      self._teardown_task:destroy()
      self._teardown_task = nil
   end
end

-- start the fabricate task if it's not already running
--
function FixtureFabricator:_start_fabricate_task() 
   if not self._fabricate_task then
      local town = stonehearth.town:get_town(self._entity)
      self._fabricate_task = town:create_worker_task('stonehearth:fabricate_fixture', { fabricator = self._entity })
                                      :set_name('fabricate')
                                      :set_source(self._entity)
                                      :set_max_workers(1)
                                      :set_priority(priorities.CONSTRUCT_BUILDING)
                                      :once()
                                      :start()
   end
end

-- destroy the fabricate task
--
function FixtureFabricator:_destroy_fabricate_task()
   if self._fabricate_task then
      self._fabricate_task:destroy()
      self._fabricate_task = nil
   end      
end


return FixtureFabricator
