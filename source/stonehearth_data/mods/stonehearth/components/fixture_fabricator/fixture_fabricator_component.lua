local build_util = require 'lib.build_util'
local entity_forms = require 'lib.entity_forms.entity_forms_lib'

local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local FixtureFabricator = class()

-- the FixtureFabricator handles the building of fixtures inside
-- structures.

-- initialize a new fixture fabricator
function FixtureFabricator:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         self:_restore()
      end)
end

function FixtureFabricator:_restore()
   if self._sv.fixture_iconic_uri then
      self._deps_listener = radiant.events.listen(self._entity, 'stonehearth:construction:dependencies_finished_changed', self, self._on_dependencies_finished_changed)
      self:_start_project()
   end
end

-- destroy the fabricator
function FixtureFabricator:destroy()
   self:_destroy_item_tracker_listener()
   self:_destroy_placeable_item_trace()

   if self._deps_listener then
      self._deps_listener:destroy()
      self._deps_listener = nil
   end
end

-- turn the fixture fabricator off or on.  we'll only try to put the
-- fixture on the parent if the parent is finished and the fabricator is
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

function FixtureFabricator:instabuild()
   if not self._sv.fixture then
      self._sv.fixture = radiant.entities.create_entity(self._sv.fixture_uri)
   end
   local root_entity = self._sv.fixture
   local parent = self._entity:get_component('mob'):get_parent()
   local normal = parent:get_component('stonehearth:construction_data')
                              :get_normal()  
   local rotation = build_util.normal_to_rotation(normal)
   local location = self._entity:get_component('mob')
                                    :get_grid_location()

   -- change ownership so we can interact with it
   root_entity:add_component('unit_info')
                  :set_player_id(radiant.entities.get_player_id(self._entity))

   radiant.entities.add_child(parent, root_entity, location)
   root_entity:add_component('mob')
                  :turn_to(rotation)

   self:_destroy_placeable_item_trace()
   self._entity:get_component('stonehearth:construction_progress')
                  :set_finished(true)
end

-- called to kick off the fabricator.  don't call until all the components are
-- installed and the fabricator entity is at the correct location in the parent
--
function FixtureFabricator:start_project(fixture, normal)
   if radiant.util.is_a(fixture, Entity) then
      fixture:get_component('stonehearth:entity_forms')
                  :set_fixture_fabricator(self._entity)
      self._sv.fixture = fixture
   end
   self._sv.normal = normal
   self._sv.fixture_uri, self._sv.fixture_iconic_uri = entity_forms.get_uris(fixture)
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

   if not finished and dependencies_finished then
      run_teardown_task = teardown
      run_fabricate_task = not teardown
   end
   
   -- Now apply the deltas.  Create tasks that need creating and destroy
   -- ones that need destroying.
   assert(not run_teardown_task)

   if run_fabricate_task then
      self:_place_fixture()
   end
end


-- start the fabricate task if it's not already running
--
function FixtureFabricator:_place_fixture() 
   if self._placing_item then
      return
   end

   local fixture = self._sv.fixture
   local location = radiant.entities.get_world_grid_location(self._entity)
   
   if not fixture then
      -- we haven't found an item to place in our fixture slot yet.  look around for
      -- one by asking the inventory for this player
      local inventory = stonehearth.inventory:get_inventory(self._entity)
      local iconic_item = inventory:find_closest_unused_placable_item(self._sv.fixture_iconic_uri, location)
      if not iconic_item then
         -- still no item exists?  listen in the basic inventory tracker for a notification that
         -- we've gotten a new item which matches the fixture uri we want, then try again
         if not self._item_tracker_listener then
            self._item_tracker_listener = inventory:notify_when_item_added(self._sv.fixture_iconic_uri, function (e)
                  self:_place_fixture()
               end)
         end
         return
      end
      fixture = iconic_item:get_component('stonehearth:iconic_form')
                              :get_root_entity()

      self._sv.fixture = fixture
      self.__saved_variables:mark_changed()
   end
                              
   -- change ownership so we can interact with it (xxx: should be unnecessary! - tony)
   fixture:add_component('unit_info')
            :set_player_id(radiant.entities.get_player_id(self._entity))

   self:_place_item_on_structure(fixture, location)

   self._placing_item = true
   self:_destroy_item_tracker_listener()
   self:_create_placeable_item_trace()
end

function FixtureFabricator:_place_item_on_structure(root_entity, location) 
   -- we've got an item!  ask it to be placed where our fixture is
   local structure = self._entity:get_component('mob')
                                 :get_parent()
                                 
   local normal = self._sv.normal
   if not normal then
      normal = structure:get_component('stonehearth:construction_data')
                           :get_normal()
   end

   root_entity:get_component('stonehearth:entity_forms')
                  :place_item_on_structure(location, structure, normal, 0)
end

function FixtureFabricator:_destroy_item_tracker_listener()
   if self._item_tracker_listener then
      self._item_tracker_listener:destroy()
      self._item_tracker_listener = nil
   end
end

function FixtureFabricator:_create_placeable_item_trace()
   if not self._placeable_item_trace and self._placing_item then
      -- whenever the placeable item shows up in the parent, mark ourselves as finished
      local parent = self._entity:get_component('mob'):get_parent()
      self._placeable_item_trace = self._sv.fixture:get_component('mob')
                                                      :trace_parent('placing fixture')

      self._placeable_item_trace:on_changed(function(new_parent)
            if new_parent == parent then
               self:_destroy_placeable_item_trace()
               self._entity:get_component('stonehearth:construction_progress')
                              :set_finished(true)
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


function FixtureFabricator:save_to_template()
   return {
      normal         = self._sv.normal,
      fixture_uri    = self._sv.fixture_uri,
   }
end

function FixtureFabricator:load_from_template(template, options, entity_map)  
   local normal = Point3(template.normal.x, template.normal.y, template.normal.z)

   radiant.events.listen_once(entity_map, 'finished_loading', function()
         stonehearth.build:add_fixture_fabricator(self._entity,
                                                  template.fixture_uri,
                                                  normal)
      end)
end

function FixtureFabricator:rotate_structure(degrees)
   local mob = self._entity:get_component('mob')
   local rotated = mob:get_location()
                           :rotated(degrees)
   mob:move_to(rotated)

   local rotation = mob:get_facing() + degrees
   if degrees >= 360 then
      degrees = degrees - 360
   end
   mob:turn_to(rotation)
end

function FixtureFabricator:layout()
   -- nothing to do...
end

function FixtureFabricator:accumulate_costs(cost)
   local uri = self._sv.fixture_uri
   local entry = cost.items[uri];
   if not entry then
      local json = radiant.resources.load_json(uri)
      local components = json and json.components
      local unit_info = components and components.unit_info or nil
      entry = {
         name = unit_info and unit_info.name or nil,
         icon = unit_info and unit_info.icon or nil,
         count = 1,
      }
      cost.items[uri] = entry
   else
      entry.count = entry.count + 1
   end
end

return FixtureFabricator
