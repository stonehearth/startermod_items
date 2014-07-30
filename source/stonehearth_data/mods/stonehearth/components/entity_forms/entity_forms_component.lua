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
   self._proxy_entity_uri = json.proxy_entity

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self:_init_commands()
      self.__saved_variables:mark_changed()
   end
end

function EntityFormsComponent:set_proxy_entity(entity)
   local self_proxy = self._sv.proxy_entity
   assert(not self_proxy or self_proxy:get_id() == entity:get_id())

   if self_proxy ~= nil then
      return
   end

   radiant.entities.set_faction(entity, self._entity)
   radiant.entities.set_player_id(entity, self._entity)
   self._sv.proxy_entity = entity
   self.__saved_variables:mark_changed()

   entity:add_component('stonehearth:iconic_form')   
                         :set_full_sized_entity(self._entity)
end

function EntityFormsComponent:get_proxy_entity()
   if not self._sv.proxy_entity then
      local proxy_entity  = radiant.entities.create_entity(self._proxy_entity_uri)
      self:set_proxy_entity(proxy_entity)
   end
   return self._sv.proxy_entity
end

--- Programatically add the 'move_item' command
-- Note that move item ideally requires the url of thep proxy, which we don't have yet.
-- Hope it gets set by the extend function (if we're going to place the item manually)
-- or the proxy component if we get this item through iconic_form
function EntityFormsComponent:_init_commands()
   local display_name = self._entity:get_component('unit_info'):get_display_name()

   local command_component = self._entity:add_component('stonehearth:commands')
   command_component:add_command('/stonehearth/data/commands/move_item')
   command_component:modify_command('move_item', function(command_data)
      command_data.event_data.full_sized_entity_uri = self._entity:get_uri()
      command_data.event_data.item_name = display_name
   end)
end

return EntityFormsComponent
