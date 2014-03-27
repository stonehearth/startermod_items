--[[
   Belongs to all objects that have been placed in the world,
   either by the player or by the DM, and that can be picked up
   again and moved.
]]

local PlacedItem = class()


-- If the proxy entity is set by the json file, add that here
-- Otherwise, it might be set by set_proxy
function PlacedItem:initialize(entity, json)
   self._entity = entity
   self._proxy_entity_uri = json.proxy_entity

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self:_init_commands()
      self.__saved_variables:mark_changed()
   end
end

function PlacedItem:set_proxy_entity(entity)   
   if self._sv.proxy_entity and self._sv.proxy_entity:is_valid() then
      if self._sv.proxy_entity:get_id() == entity:get_id() then
         return
      end
      radiant.entities.destroy_entity(self._sv.proxy_entity)
   end
   radiant.entities.set_faction(entity, self._entity)
   self._sv.proxy_entity = entity
   self.__saved_variables:mark_changed()

   entity:add_component('stonehearth:placeable_item_proxy')   
                         :set_full_sized_entity(self._entity)
end

function PlacedItem:get_proxy_entity()
   if not self._sv.proxy_entity then
      local proxy_entity  = radiant.entities.create_entity(self._proxy_entity_uri)
      self:set_proxy_entity(proxy_entity)
   end
   return self._sv.proxy_entity
end

--- Programatically add the 'move_item' command
-- Note that move item ideally requires the url of thep proxy, which we don't have yet.
-- Hope it gets set by the extend function (if we're going to place the item manually)
-- or the proxy component if we get this item through placeable_item_proxy
function PlacedItem:_init_commands()
   local display_name = self._entity:get_component('unit_info'):get_display_name()

   local command_component = self._entity:add_component('stonehearth:commands')
   command_component:add_command('/stonehearth/data/commands/move_item')
   command_component:modify_command('move_item', function(command_data)
      command_data.event_data.full_sized_entity_uri = self._entity:get_uri()
      command_data.event_data.item_name = display_name
   end)
end

return PlacedItem
