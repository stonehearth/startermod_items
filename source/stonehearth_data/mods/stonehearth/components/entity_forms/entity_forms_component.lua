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
      self:_create_traces()
   else
      self._sv._initialized = true
      self._sv.placeable_on_ground = json.placeable_on_ground

      self.__saved_variables:mark_changed()

      radiant.events.listen_once(entity, 'radiant:entity:post_create', function()
            local placeable = json.placeable_on_ground

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
               
               self._entity:add_component('stonehearth:commands')
                              :add_command('/stonehearth/data/commands/move_item')
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
         end)
   end
end


function EntityFormsComponent:destroy()
   if self._unit_info_trace then
      self._unit_info_trace:destroy()
      self._unit_info_trace = nil
   end
   if self._placement_task then
      self._placement_task:destroy()
      self._placement_task = nil
   end
end

function EntityFormsComponent:place_item_in_world(location, rotation)
   assert(self._sv.placeable_on_ground, 'cannot place unplacable item')
   assert(self._sv.iconic_entity, 'cannot place items with no iconic entity')
   assert(self._sv.ghost_entity, 'cannot place items with no ghost entity')
   
   if self._placement_task then
      self._placement_task:destroy()
      self._placement_task = nil
   end

   local town = stonehearth.town:get_town(self._entity)
   if town then
      local item = self:_get_form_in_world()
      assert(item, 'neither entity nor iconic form is in world')

      radiant.terrain.place_entity_at_exact_location(self._sv.ghost_entity, location)
      radiant.entities.turn_to(self._sv.ghost_entity, rotation)
      
      self._placement_task = town:create_task_for_group('stonehearth:task_group:placement', 'stonehearth:place_item', {
                                                item = item,
                                                location = location,
                                                rotation = rotation,
                                             })

      self._placement_task:set_priority(stonehearth.constants.priorities.worker_task.PLACE_ITEM)                    
                          :once()
                          :notify_completed(function()
                                 radiant.terrain.remove_entity(self._sv.ghost_entity)
                              end)
                          :start()
      self._sv._create_placement_task = true
   end
end

function EntityFormsComponent:get_iconic_entity()
   return self._sv.iconic_entity
end

function EntityFormsComponent:get_ghost_entity()
   return self._sv.ghost_entity
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

function EntityFormsComponent:_create_traces()
   local function forward_unit_info(other_entity)
      if other_entity then
         local unit_info = self._entity:get_component('unit_info')
         other_entity:add_component('unit_info')
                           :set_player_id(unit_info:get_player_id())
                           :set_faction(unit_info:get_faction())
                           :set_kingdom(unit_info:get_kingdom())
      end
   end

   assert(not self._unit_info_trace, 'trace should be installed exactly once')

   local unit_info = self._sv.root_entity:add_component('unit_info')
   self._unit_info_trace = unit_info:trace('copying unit info')
                     :on_changed(function()
                           forward_unit_info(self._sv.ghost_entity)
                           forward_unit_info(self._sv.iconic_entity)
                        end)
                     :push_object_state()
end

return EntityFormsComponent
