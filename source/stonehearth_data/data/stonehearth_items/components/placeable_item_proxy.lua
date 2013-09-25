--[[
   Placeable_item_proxy.lua
   Some items are created in the world and immediately exist.
   Other items are created as "icons" and don't have the properties of
   their final state until they are placed in the world by the player.
   This component represents one of those items before it is placed.
   It contains a link to the real-world entity it is linked to.
]]

local PlaceableItemProxy = class()

function PlaceableItemProxy:__init(entity, data_binding)
   self._entity = entity               --The 1x1 placeable cube
   self._full_sized_entity = nil       --Onced initialized, the actual full-sized entity

   self._data = data_binding:get_data()
   self._data.entity_id = entity:get_id()

   self._data_binding = data_binding
   self._data_binding:mark_changed()
end

function PlaceableItemProxy:extend(json)
   if json and json.full_sized_entity then
      self._data.full_sized_entity_uri = json.full_sized_entity;
      self._data_binding:mark_changed()

      self:_create_derived_components()
   end
end

function PlaceableItemProxy:get_full_sized_entity()
   if not self._full_sized_entity then
      self:_create_full_sized_entity();
   end
   return self._full_sized_entity
end

--Get the uri of the full sized entity to the place command
function PlaceableItemProxy:get_full_sized_entity_uri()
   return self._data.full_sized_entity_uri
end

--[[
   Create the entity and extract our own unit info from its unit info.
]]
function PlaceableItemProxy:_create_full_sized_entity()
   self._full_sized_entity = radiant.entities.create_entity(self._data.full_sized_entity_uri)
   self._full_sized_entity:add_component('stonehearth_items:placeable_item_pointer'):set_proxy(self._entity)
end

--Copy the unit info from the big entity's json file into the proxy
function PlaceableItemProxy:_create_derived_components()
   local full_sized_uri = self:get_full_sized_entity_uri()
   local display_name = ''

   local json = radiant.resources.load_json(full_sized_uri)
   if json and json.components then
      if json.components.unit_info then
         local data = json.components.unit_info
         local unit_info = self._entity:add_component('unit_info')
         unit_info:set_display_name(data.name and data.name or '')
         unit_info:set_description(data.description and data.description or '')
         unit_info:set_icon(data.icon and data.icon or '')
         display_name  = unit_info:get_display_name();
      end
      if json.components.item then
         local data = json.components.unit_info
         local item = self._entity:add_component('item')
         item:set_material(data.material and data.material or '')
         item:set_category(data.category and data.category or '')
         item:set_identifier(data.identifier and data.identifier or '')
      end
   end
   --Issues: if this is in a parent class, it isn't loaded by this point, so add manually
   --local place_command = self._entity:get_component('radiant:commands')
   local place_command = self._entity:add_component('radiant:commands')
   place_command:add_command(radiant.resources.load_json('/stonehearth_items/commands/place_command.json'))



   local command_data = place_command:modify_command('place_item')
   command_data.event_data.full_sized_entity_uri = full_sized_uri
   command_data.event_data.proxy = self._data_binding
   command_data.event_data.item_name = display_name
end

return PlaceableItemProxy