local priorities = require('constants').priorities.simple_labor

local FixtureFabricator = class()

-- the FixtureFabricator handles the building of fixtures inside
-- structures.

-- initialize a new fixture fabricator
function FixtureFabricator:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         if self._sv.fixture_iconic_uri then
            radiant.events.listen(self._entity, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)
            self:_start_project()
         end
      end)
end

-- destroy the fabricator
function FixtureFabricator:destroy()
   self:_destroy_item_tracker_listener()
   self:_destroy_placeable_item_trace()
   radiant.events.unlisten(self._entity, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)
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

-- not used
function FixtureFabricator:get_project()
   return nil
end

function FixtureFabricator:set_teardown()
   -- noop.
end

-- called to kick off the fabricator.  don't call until all the components are
-- installed and the fabricator entity is at the correct location in the wall
--
function FixtureFabricator:start_project(fixture_iconic_uri)
   assert(fixture_iconic_uri)

   self._sv.fixture_iconic_uri = fixture_iconic_uri
   self.__saved_variables:mark_changed()
   
   radiant.events.listen(self._entity, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)  
   self:_start_project()
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
   assert(not run_teardown_task)

   if run_fabricate_task then
      self:_place_fixture_in_wall()
      self:_create_placeable_item_trace()
   end
end


-- start the fabricate task if it's not already running
--
function FixtureFabricator:_place_fixture_in_wall() 
   if not self._sv.placing_item then
      -- we haven't found an item to place in our fixture slot yet.  look around for
      -- one by asking the inventory for this player
      local location = radiant.entities.get_world_grid_location(self._entity)
      local inventory = stonehearth.inventory:get_inventory(self._entity)
      local item  = inventory:find_closest_unused_placable_item(self._sv.fixture_iconic_uri, location)

      if not item then
         -- still no item exists?  listen in the basic inventory tracker for a notification that
         -- we've gotten a new item which matches the fixture uri we want, then try again
         if not self._item_tracker_listener then
            self._item_tracker_listener = inventory:notify_when_item_added(self._sv.fixture_iconic_uri, function (e)
                  self:_place_fixture_in_wall()
                  self:_create_placeable_item_trace()
               end)
         end
         return
      end

      -- we've got an item!  ask it to be placed where our fixture is      
      local wall = self._entity:get_component('mob'):get_parent()
      local normal = wall:get_component('stonehearth:construction_data')
                              :get_normal()
      
      local root_entity = item:get_component('stonehearth:iconic_form')
                                 :get_root_entity()
                                 
      -- change ownership so we can interact with it
      root_entity:add_component('unit_info')
                     :set_player_id(radiant.entities.get_player_id(self._entity))
                     :set_faction(radiant.entities.get_faction(self._entity))

      root_entity:get_component('stonehearth:entity_forms')
                     :place_item_on_wall(location, wall, normal)

      self._sv.placing_item = item
      self.__saved_variables:mark_changed()
      self:_destroy_item_tracker_listener()
   end
end

function FixtureFabricator:_destroy_item_tracker_listener()
   if self._item_tracker_listener then
      self._item_tracker_listener:destroy()
      self._item_tracker_listener = nil
   end
end

function FixtureFabricator:_create_placeable_item_trace()
   if not self._placeable_item_trace and self._sv.placing_item then
      local root_entity = self._sv.placing_item:get_component('stonehearth:iconic_form')
                                                   :get_root_entity()

      -- whenever the placeable item shows up in the wall, destroy ourselves (since
      -- our work will be done)
      local wall = self._entity:get_component('mob'):get_parent()
      self._placeable_item_trace = root_entity:get_component('mob'):trace_parent('placing fixture')
                                             :on_changed(function(parent)
                                                   if parent == wall then
                                                      self:_destroy_placeable_item_trace()
                                                      radiant.entities.destroy_entity(self._entity)
                                                   end                                                   
                                                end)
   end
end

function FixtureFabricator:_destroy_placeable_item_trace()   
   if self._placeable_item_trace then
      self._placeable_item_trace:destroy()
      self._placeable_item_trace = nil
   end
end

return FixtureFabricator
