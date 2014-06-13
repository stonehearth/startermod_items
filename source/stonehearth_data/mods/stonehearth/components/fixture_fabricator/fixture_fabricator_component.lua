local priorities = require('constants').priorities.worker_task

local FixtureFabricator = class()

-- the FixtureFabricator handles the building of fixtures inside
-- structures.

-- initialize a new fixture fabricator
function FixtureFabricator:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         if self._sv.fixture_uri then
            radiant.events.listen(self._entity, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)
         end
      end)

end

-- destroy the fabricator
function FixtureFabricator:destroy()
   radiant.events.unlisten(self._entity, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)   
end

-- returns the project (i.e. the build fixture)
--
function FixtureFabricator:get_project()
   return self._sv.project
end

-- turn the fixture fabricator off or on.  we'll only try to put the
-- fixture on the wall if the wall is finished and the fabricator is
-- active
-- 
--    @param enabled - whether or not to try to build the fixture
--
function FixtureFabricator:set_active(enabled)
   self:_start_project()
end

-- called to kick off the fabricator.  don't call until all the components are
-- installed and the fabricator entity is at the correct location in the wall
--
function FixtureFabricator:start_project(fixture_uri)
   assert(fixture_uri)

   self._sv.fixture_uri = fixture_uri
   self.__saved_variables:mark_changed()
   
   radiant.events.listen(self._entity, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)
   
   self:_borrow_scaffolding_from_parent()
   self:_start_project()
end   

-- helper function called by the fabricate action to actually build the fixture.
--
function FixtureFabricator:construct(worker, item)
   local project, uri

   -- xxx: for now, create a brand new entity.  eventually, we should simply be
   -- replacing the ghost_entity with the full_sized_entity for the proxy.
   local proxy_component = item:get_component('stonehearth:placeable_item_proxy')
   if proxy_component then
      project = proxy_component:get_full_sized_entity()
   else
      project = item
   end

   -- take the proxy off the current worker or the terrain, depending on where
   -- it came from.
   if radiant.entities.get_carrying(worker) == item then
      radiant.entities.remove_carrying(worker)
   else
      radiant.terrain.remove_entity(item)
   end

   -- find the project for our parent
   local mob = self._entity:get_component('mob')
   local parent_blueprint = mob:get_parent()
   local parent_project = parent_blueprint:get_component('stonehearth:construction_progress')
                                 :get_fabricator_component()
                                 :get_project()

   radiant.entities.add_child(parent_project, project)
   project:add_component('mob'):set_transform(mob:get_transform())

   -- change ownership so we can interact with it
   project:add_component('unit_info')
               :set_player_id(radiant.entities.get_player_id(self._entity))
               :set_faction(radiant.entities.get_faction(self._entity))

   -- all done!  set a flag so we know and remember the project.
   self._sv.project = project
   self._sv.finished = true
   self._entity:get_component('stonehearth:construction_progress'):set_finished(true)

   self.__saved_variables:mark_changed()
end

-- check the status of the project again whenever the status of our dependencies change.
function FixtureFabricator:_on_dependencies_finished_changed()
   self:_start_project()
end

-- either create or destroy the tasks depending on our current state
--
function FixtureFabricator:_start_project()
   local run_teardown_task, run_fabricate_task = false, false

   local cp = self._entity:get_component('stonehearth:construction_progress')
   local active = cp:get_active()
   local finished = cp:get_finished()
   local teardown = cp:get_teardown()
   local dependencies_finished = cp:get_dependencies_finished()

   if not finished and active and dependencies_finished then
      run_teardown_task = teardown
      run_fabricate_task = not teardown
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
      self._fabricate_task = town:create_worker_task('stonehearth:fabricate_fixture', {
                                             fabricator = self._entity,
                                             fixture_uri = self._sv.fixture_uri 
                                          })
                                      :set_name('fabricate fixture')
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

-- borrow the scaffolding of our parent.  this makes sure the build system keeps
-- the scaffolding at our height during teardown so someone can come along and place
-- the fixture on the entity.  this only works for things which go on the scaffolding
-- side of the parent.  other fixture fabricators need to use a ladder.
--
function FixtureFabricator:_borrow_scaffolding_from_parent()
   local parent_blueprint = self._entity:get_component('mob'):get_parent()
   if parent_blueprint then
      local cd = parent_blueprint:get_component('stonehearth:construction_data')
      if cd then
         cd:loan_scaffolding_to(self._entity)
      end
   end
end

return FixtureFabricator
