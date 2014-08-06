local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3

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
   
   local placeable = self:is_placeable()

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
      
      if radiant.is_server then
         self._entity:add_component('stonehearth:commands')
                        :add_command('/stonehearth/data/commands/move_item')
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
                        :set_placeable(placeable)
                        :set_root_entity(self._entity)
                        :set_ghost_entity(ghost_entity)
   end
   if ghost_entity then
      ghost_entity:add_component('stonehearth:ghost_form')
                        :set_placeable(placeable)
                        :set_root_entity(self._entity)
                        :set_iconic_entity(iconic_entity)
   end
end

function EntityFormsComponent:destroy()
   self:_destroy_placement_task()
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
   return self._placement_task ~= nil
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
      if self._sv.placing_at.wall then
         self:place_item_on_wall(self._sv.placing_at.wall,
                                 self._sv.placing_at.location,
                                 self._sv.placing_at.normal)
      else
         self:place_item_in_world(self._sv.placing_at.location,
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
   if self._wall_finished_event then
      self._wall_finished_event:destroy()
      self._wall_finished_event = nil
   end
   if self._sv.placing_at then
      self._sv.placing_at = nil
      self.__saved_variables:mark_changed()
   end
end

function EntityFormsComponent:place_item_on_wall(location, wall_entity, normal)
   assert(self:is_placeable_on_wall(), 'cannot place item on wall')

   -- cancel whatever pending tasks we had earliers.
   self:_destroy_placement_task()

   -- rememeber what we're doing now so we can early-exit later if necessary.
   self._sv.placing_at = {
      wall = wall_entity,
      normal = normal,
      location = location,
   }
   self.__saved_variables:mark_changed()

   -- figure out if the wall is finished or not.  we shouldn't try to place items on
   -- unfinished walls..
   local wall_cp = wall_entity:get_component('stonehearth:construction_progress')
   local wall_finished = wall_cp:get_finished()

   -- move our ghost entity inside the wall that we'd like to be placed on.  this
   -- gives the wall ownership of the visibilty of the ghost item (e.g. when the
   -- build view mode changes, the ghost item will show and hide itself along with
   -- the wall).  if the wall's finished, we'll use the blueprint.  otherwise,
   -- use the fabricator
   local rotation = voxel_brush_util.normal_to_rotation(normal)
   local offset = location - radiant.entities.get_world_grid_location(wall_entity)
   local ghost_entity_parent = wall_finished and wall_entity or wall_cp:get_fabricator_entity()

   radiant.entities.add_child(ghost_entity_parent, self._sv.ghost_entity, offset)
   radiant.entities.turn_to(self._sv.ghost_entity, rotation)

   if not wall_finished then
      radiant.events.listen_once(wall_entity, 'stonehearth:construction:finished_changed', function()
            self._wall_finished_event = self:place_item_on_wall(location, wall_entity, normal)
         end)
      return
   end

   radiant.entities.add_child(wall_entity, self._sv.ghost_entity, offset)
   radiant.entities.turn_to(self._sv.ghost_entity, rotation)

   -- the wall is finished.  let's start up the tasks.  if we're currently on a wall,
   -- we may need to build a ladder to pick ourselves up.  request that now.
   local parent = self._entity:get_component('mob'):get_parent()
   if parent and parent:get_component('stonehearth:wall') then
      local parent_normal = parent:get_component('stonehearth:construction_data')
                                       :get_normal()
      local pickup_location = radiant.entities.get_world_grid_location(self._entity) + parent_normal
      self._climb_to_item = stonehearth.build:request_ladder_to(pickup_location, parent_normal)
   end

   -- create another ladder request to climb up to the placement point.
   local climb_to = normal + location - Point3.unit_y
   self._climb_to_destination = stonehearth.build:request_ladder_to(climb_to, normal)

   -- finally create a placement task to move the item from one spot to another
   local item = self:_get_form_in_world()
   self:_create_task('stonehearth:place_item_on_wall', {
         item = item,
         wall = wall_entity,
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
   self._placement_task:set_priority(stonehearth.constants.priorities.worker_task.PLACE_ITEM)                    
                       :once()
                       :notify_completed(function()
                              radiant.terrain.remove_entity(self._sv.ghost_entity)
                              self._placement_task = nil
                              self:_destroy_placement_task()
                           end)
                       :start()
end

return EntityFormsComponent
