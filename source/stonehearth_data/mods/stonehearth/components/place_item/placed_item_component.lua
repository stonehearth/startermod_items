--[[
   Belongs to all objects that have been placed in the world,
   either by the player or by the DM, and that can be picked up
   again and moved.
]]

local PlacedItem = class()

function PlacedItem:__init(entity, data_store)
   self._entity = entity

   self._proxy_uri = nil  --Hopefully this is set by the time the user tries to move the item again

   self:_init_commands()
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

-- If the proxy entity is set by the json file, add that here
-- Otherwise, it might be set by set_proxy
function PlacedItem:extend(json)
   if json and json.proxy_entity then
      self._proxy_uri = json.proxy_entity
   end
end

function PlacedItem:set_proxy(proxy_entity)
   self._proxy_uri = proxy_entity:get_uri()
end

function PlacedItem:get_proxy()
   return self._proxy_uri
end

return PlacedItem