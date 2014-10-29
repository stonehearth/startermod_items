local build_util = require 'lib.build_util'
local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

--[[
   Belongs to all objects that have been placed in the world,
   either by the player or by the DM, and that can be picked up
   again and moved.
]]

local EntityFormsComponent = class()


-- If the proxy entity is set by the json file, add that here
-- Otherwise, it might be set by set_proxy
function EntityFormsComponent:initialize(entity, json)
   self._entity = entity

   if self._sv._initialized then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            self:_load_placement_task()
            if self._sv.should_restock then
               -- why do we need to set a 1000 ms timer just to get the effect to show up?
               -- this is quite disturbing... investigate! -- tony
               radiant.set_realtime_timer(1000, function()
                     self:_update_restock_info()
                  end)
            end
         end)

   else
      self.__saved_variables:mark_changed()
      radiant.events.listen_once(entity, 'radiant:entity:post_create', function()
            self:_post_create(json)
         end)
   end
end

function EntityFormsComponent:_post_create(json)
   self._sv._initialized = true
   self._sv.placeable_on_walls = json.placeable_on_walls
   self._sv.placeable_on_ground = json.placeable_on_ground
   self._sv.hide_placement_ui = json.hide_placement_ui

   local placeable = self:is_placeable()
   local show_placement_ui = placeable and not self:should_hide_placement_ui()

   local iconic_entity, ghost_entity
   if json.iconic_form then
      iconic_entity = radiant.entities.create_entity(json.iconic_form)
   end
   local ghost_form = json.ghost_form
   if json.ghost_form then
      ghost_entity = radiant.entities.create_entity(json.ghost_form)
   end

   if ghost_entity then
      ghost_entity:add_component('render_info')
                     :set_material('materials/ghost_item.xml')
   end

   if placeable then
      local uri = self._entity:get_uri()
      assert(iconic_entity, string.format('placeable entity %s missing an iconic entity form', uri))
      assert(ghost_entity,  string.format('placeable entity %s missing a ghost entity form', uri))
      
      if radiant.is_server and show_placement_ui then
         local commands_component = self._entity:add_component('stonehearth:commands')
         commands_component:add_command('/stonehearth/data/commands/move_item')
         commands_component:add_command('/stonehearth/data/commands/undeploy_item')
      end
   end
   
   if placeable then
      self._sv.placeable_category = json.placeable_category or 'uncategorized'
   end
   self._sv.iconic_entity = iconic_entity
   self._sv.ghost_entity = ghost_entity
   self.__saved_variables:mark_changed()

   if iconic_entity then
      iconic_entity:add_component('stonehearth:iconic_form')
                        :set_placeable(show_placement_ui)
                        :set_root_entity(self._entity)
                        :set_ghost_entity(ghost_entity)
   end
   if ghost_entity then
      ghost_entity:add_component('stonehearth:ghost_form')
                        :set_placeable(show_placement_ui)
                        :set_root_entity(self._entity)
                        :set_iconic_entity(iconic_entity)
   end
end

function EntityFormsComponent:destroy()
   self:_destroy_placement_task()
end

-- whether this item, if it is deployed, should be undeployed and stored in a stockpile
function EntityFormsComponent:get_should_restock()
   return self._sv.should_restock
end

-- call to toggle whether this deployed item should be undeployed and stored in a stockpile
function EntityFormsComponent:set_should_restock(restock)
   if restock ~= self._sv.should_restock then
      self._sv.should_restock = restock
      self.__saved_variables:mark_changed()
      self:_update_restock_info()
   end
end

function EntityFormsComponent:set_fixture_fabricator(fixture_fabricator)
   if self._sv.fixture_fabricator then
      radiant.entities.destroy_entity(self._sv.fixture_fabricator)
   end
   self._sv.fixture_fabricator = fixture_fabricator
   self.__saved_variables:mark_changed()
end

function EntityFormsComponent:_update_restock_info()
   if self._sv.should_restock then 
      if not self._overlay_effect then
         self._overlay_effect = radiant.effects.run_effect(self._entity, '/stonehearth/data/effects/undeploy_overlay_effect');
      end

      -- trace the item's position. When it's position becomes nil, we know the item has changed
      -- forms, so if necessary we'll clear the undeploy setting
      if not self._position_trace then
         self._position_trace = radiant.entities.trace_location(self._entity, 'entity forms component')
                                                :on_changed(function()
                                                   self:_on_position_changed()
                                                end)
      end
   else
      if self._overlay_effect then
         self._overlay_effect:stop()
         self._overlay_effect = nil
      end

      if self._position_trace then
         self._position_trace:destroy()
      end
   end
end

function EntityFormsComponent:should_hide_placement_ui()
   return self._sv.hide_placement_ui
end

function EntityFormsComponent:is_placeable()
   return self._sv.placeable_on_ground or
          self._sv.placeable_on_walls
end

function EntityFormsComponent:is_placeable_on_wall()
   return self._sv.placeable_on_walls
end

function EntityFormsComponent:is_placeable_on_ground()
   return self._sv.placeable_on_ground
end

function EntityFormsComponent:is_being_placed()
   -- use the saved variables as the authorative source as to whether or not
   -- we're being placed.  this avoids some races at load time (e.g. what if
   -- someone asks before we've had the opportunity to create our task?)
   return self._sv.placing_at ~= nil
end

function EntityFormsComponent:get_placeable_category()
   return self._sv.placeable_category
end

function EntityFormsComponent:get_iconic_entity()
   return self._sv.iconic_entity
end

function EntityFormsComponent:get_ghost_entity()
   return self._sv.ghost_entity
end

function EntityFormsComponent:_load_placement_task()
   if self._sv.placing_at then
      if self._sv.placing_at.structure then
         -- the fabricator will restart us...
      else
         self:place_item_on_ground(self._sv.placing_at.location,
                                   self._sv.placing_at.rotation)
      end   
   end
end

function EntityFormsComponent:_destroy_placement_task()
   if self._climb_to_item then
      self._climb_to_item:destroy()
      self._climb_to_item = nil
   end
   if self._climb_to_destination then
      self._climb_to_destination:destroy()
      self._climb_to_destination = nil
   end
   if self._placement_task then
      self._placement_task:destroy()
      self._placement_task = nil
   end
   if self._structure_finished_event then
      self._structure_finished_event:destroy()
      self._structure_finished_event = nil
   end
   if self._sv.placing_at then
      self._sv.placing_at = nil
      self.__saved_variables:mark_changed()
   end
end

function EntityFormsComponent:place_item_on_structure(location, structure_entity, normal, rotation)
   assert(structure_entity, 'no structure in :place_item_on_structure()')
   assert(normal, 'no normal in :place_item_on_structure()')
   assert(radiant.util.is_a(structure_entity, Entity), 'structure is not an entity in place_item_on_structure')
   assert(self:is_placeable(), 'cannot place non-placeable item')

   -- cancel whatever pending tasks we had earliers.
   self:_destroy_placement_task()

   -- rememeber what we're doing now so we can early-exit later if necessary.
   self._sv.placing_at = {
      structure = structure_entity,
      normal = normal,
      location = location,
      rotation = rotation,
   }
   self.__saved_variables:mark_changed()

   -- make sure the structure is finished, first.  the fixture fabricator should have
   -- managed this dependency for us!
   local structure_cp = structure_entity:get_component('stonehearth:construction_progress')
   assert(structure_cp:get_finished())

   -- the structure is finished.  let's start up the tasks.  if we're currently on a structure,
   -- we may need to build a ladder to pick ourselves up.  request that now.
   local mob = self._entity:add_component('mob')
   local parent = mob:get_parent()
   if parent and parent:get_component('stonehearth:wall') then
      -- ah!  we're on a wall.  the ladder has to move over by our normal to the wall, which
      -- we can compute based on the rotation.
      local current_rotation = mob:get_facing()
      local curernt_normal = build_util.rotation_to_normal(current_rotation)
      local pickup_location = radiant.entities.get_world_grid_location(self._entity) + curernt_normal
      self._climb_to_item = stonehearth.build:request_ladder_to(pickup_location, curernt_normal)
   end

   -- create another ladder request to climb up to the placement point.
   if normal and normal.y == 0 then
      local climb_to = normal + location - Point3.unit_y
      self._climb_to_destination = stonehearth.build:request_ladder_to(climb_to, normal)
   end

   -- finally create a placement task to move the item from one spot to another
   local item = self:_get_form_in_world()
   self:_create_task('stonehearth:place_item_on_structure', {
         item = item,
         structure = structure_entity,
         location = location,
         rotation = rotation,
      })
end

function EntityFormsComponent:place_item_on_ground(location, rotation)
   assert(self:is_placeable_on_ground(), 'cannot place item on ground')
   assert(self._sv.iconic_entity, 'cannot place items with no iconic entity')
   assert(self._sv.ghost_entity, 'cannot place items with no ghost entity')
   
   self:_destroy_placement_task()

   self._sv.placing_at = {
      location = location,
      rotation = rotation,
   }
   self.__saved_variables:mark_changed()

   local item = self:_get_form_in_world()
   assert(item, 'neither entity nor iconic form is in world')

   radiant.terrain.place_entity_at_exact_location(self._sv.ghost_entity, location)
   radiant.entities.turn_to(self._sv.ghost_entity, rotation)

   self:_create_task('stonehearth:place_item', {
         item = item,
         location = location,
         rotation = rotation,
      })
end

function EntityFormsComponent:_get_form_in_world()
   local parent = self._entity:get_component('mob'):get_parent()
   if parent then
      return self._entity
   end
   if self._sv.iconic_entity then
      parent = self._sv.iconic_entity:get_component('mob'):get_parent()
      if parent then
         return self._sv.iconic_entity
      end
   end
   return nil
end

function EntityFormsComponent:_create_task(activity, args)
   local town = stonehearth.town:get_town(self._entity)

   assert(town)
   assert(not self._placement_task)  

   self._placement_task = town:create_task_for_group('stonehearth:task_group:placement', activity, args)
   self._placement_task:set_priority(stonehearth.constants.priorities.simple_labor.PLACE_ITEM)                    
                       :once()
                       :notify_completed(function()
                              -- remove the ghost entity from whatever its parent was.  this could
                              -- best the terrain or a structure, depending on how we were placed
                              local ghost = self._sv.ghost_entity
                              local parent = radiant.entities.get_parent(ghost)
                              if parent then
                                 radiant.entities.remove_child(parent, ghost)
                                 radiant.entities.move_to(ghost, Point3.zero)                              
                              end
                              self._placement_task = nil
                              self:_destroy_placement_task()
                           end)
                       :add_entity_effect(self._entity, '/stonehearth/data/effects/move_item_overlay_effect')
                       :start()
end

function EntityFormsComponent:_on_position_changed()
   if radiant.entities.get_world_grid_location(self._entity) == nil then
      self:set_should_restock(false)
   end
end


return EntityFormsComponent
