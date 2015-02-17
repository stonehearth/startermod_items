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
   if self._auto_destroy_trace then
      self._auto_destroy_trace:destroy()
      self._auto_destroy_trace = nil
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

function FixtureFabricator:get_normal()
   return self._sv.normal
end

function FixtureFabricator:get_rotation()
   return self._sv.rotation
end

function FixtureFabricator:get_uri()
   return self._sv.fixture_uri
end

function FixtureFabricator:instabuild()
   if not self._sv.fixture then
      self._sv.fixture = radiant.entities.create_entity(self._sv.fixture_uri, { owner = self._entity })
   end
   local root_entity = self._sv.fixture
   local parent = self._entity:get_component('mob'):get_parent()
   local normal = parent:get_component('stonehearth:construction_data')
                              :get_normal()  
   local location = self._entity:get_component('mob')
                                    :get_grid_location()

   -- change ownership so we can interact with it
   root_entity:add_component('unit_info')
                  :set_player_id(radiant.entities.get_player_id(self._entity))

   local rotation
   if normal then
      rotation = build_util.normal_to_rotation(normal)
   else
      rotation = self._entity:get_component('mob')
                                 :get_facing()
   end
   root_entity:add_component('mob')
                  :turn_to(rotation)
   radiant.entities.add_child(parent, root_entity, location)

   self:_destroy_placeable_item_trace()
   self:_set_finished()
end

-- called to kick off the fabricator.  don't call until all the components are
-- installed and the fabricator entity is at the correct location in the parent
--
function FixtureFabricator:start_project(fixture, normal, rotation)
   if radiant.util.is_a(fixture, Entity) then
      fixture:get_component('stonehearth:entity_forms')
                  :set_fixture_fabricator(self._entity)
      self._sv.fixture = fixture
   end
   self._sv.normal = normal
   self._sv.rotation = rotation
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
   local rotation = self._sv.rotation
   if not rotation then
      rotation = build_util.normal_to_rotation(normal)
   end

   root_entity:get_component('stonehearth:entity_forms')
                  :place_item_on_structure(location, structure, normal, rotation)
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
               self:_set_finished()
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
      rotation       = self._sv.rotation,
      fixture_uri    = self._sv.fixture_uri,
   }
end

function FixtureFabricator:load_from_template(template, options, entity_map)  
   self._template = template
end

function FixtureFabricator:finish_restoring_template()
   local template = self._template
   self._template = nil
   
   local normal = Point3(template.normal.x, template.normal.y, template.normal.z)
   local rotation = template.rotation
   
   if self._post_load_rotation then
      normal = normal:rotated(self._post_load_rotation)
      if rotation then
         rotation = build_util.rotated_degrees(rotation, self._post_load_rotation)
      end
      self._post_load_rotation = nil
   end
   stonehearth.build:add_fixture_fabricator(self._entity,
                                            template.fixture_uri,
                                            normal,
                                            rotation)
end

function FixtureFabricator:rotate_structure(degrees)
   local mob = self._entity:get_component('mob')
   local location = mob:get_location()
   local rotated = location:rotated(degrees)

   if degrees == 270 or degrees == 180 then
      --
      -- what the heck is going on here?  basically other parts of the system want to
      -- maintain the invariant that square region's maintain their same position in
      -- the world when rotated, even if the origin is not exactly at the center of
      -- the region.  this is accomplished by the axis alignment flags on the mob
      -- component.
      --
      -- this means that when an entity is rotated beyond 180 degress, the origin
      -- flips to the other side of the region (e.g. -1, 0 -> 0, 1).  to account
      -- for this, move the fixture along the wall by an opposite amount.
      --
      -- yes, this is very "voodoo magic".  i apologize and loathe myself for writing
      -- it. - tony
      --
      local alignment = mob:get_align_to_grid_flags()
      if alignment ~= 0 then
         local wall = mob:get_parent()
                           :get_component('stonehearth:wall')
         if wall then
            local t = wall:get_tangent_coord()
            rotated[t] = rotated[t] + 1
         end
      end
   end

   mob:move_to(rotated)

   local rotation = build_util.rotated_degrees(mob:get_facing(), degrees)
   mob:turn_to(rotation)


   self._post_load_rotation = degrees
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

function FixtureFabricator:_set_finished()
   self:_destroy_placeable_item_trace()

   self._entity:get_component('stonehearth:construction_progress')
                  :set_finished(true)

   self:_update_auto_destroy_trace()
end

function FixtureFabricator:_update_auto_destroy_trace()
   assert(self._sv.fixture)

   -- now that we're built, if the bed somehow moves off the building, remove the
   -- blueprint from the building.  this can happen when people start re-arranging
   -- furniture on already built houses.
   --
   self._auto_destroy_trace = self._sv.fixture:get_component('mob')
                                                   :trace_parent('placing fixture')

   local parent = self._entity:get_component('mob')
                                 :get_parent()                                    
   local old_location = self._sv.fixture:get_component('mob')
                                             :get_location()
   self._auto_destroy_trace:on_changed(function(new_parent)
         if new_parent ~= parent then
            local location = self._sv.fixture:get_component('mob')
                                                :get_location()
            if location ~= old_location then
               self._auto_destroy_trace:destroy()
               self._auto_destroy_trace = nil
               radiant.entities.destroy_entity(self._entity)
            end
         end                                                   
      end)
end

return FixtureFabricator
