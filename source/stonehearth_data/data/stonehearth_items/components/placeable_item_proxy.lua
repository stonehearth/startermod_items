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
   self._full_sized_entity_uri = ''    --Where to find a description of the full sized entity
   self._full_sized_entity = nil       --Onced initialized, the actual full-sized entity

   self._data = data_binding:get_data()
   self._data.entity_id = entity:get_id()
   self._data.full_sized_entity_mod = nil
   self._data.full_sized_entity_name = nil

   self._data_binding = data_binding
   self._data_binding:mark_changed()
end

function PlaceableItemProxy:extend(json)
   if json and json.full_sized_entity_mod and json.full_sized_entity_name then
      self._data.full_sized_entity_mod = json.full_sized_entity_mod
      self._data.full_sized_entity_name = json.full_sized_entity_name
      self:_create_full_sized_entity()
      self._data_binding:mark_changed()
   end
end

function PlaceableItemProxy:get_full_sized_entity()
   return self._full_sized_entity
end

--function PlaceableItemProxy:get_full_sized_entity_uri()
--   return self._full_sized_entity_uri
--end

--[[
   Create the entity and extract our own unit info from its unit info.
]]
function PlaceableItemProxy:_create_full_sized_entity()
   self._full_sized_entity = radiant.entities.create_entity(self._data.full_sized_entity_mod, self._data.full_sized_entity_name) --radiant.entities.create_entity(self._full_sized_entity_uri)
   self._full_sized_entity:add_component('stonehearth_items:placeable_item_pointer'):set_proxy(self._entity)
   self:_create_unit_info()
   self:_create_item_data()

   --Get the uri of the full sized entity to the place command
   local place_command = self._entity:get_component('radiant:commands')
   place_command:add_event_data('place_item', 'target_mod', self._data.full_sized_entity_mod)
   place_command:add_event_data('place_item', 'target_name', self._data.full_sized_entity_name)
   place_command:add_event_data('place_item', 'item_name', self._display_name)
end

--Copy the unit info from the big entity into the proxy
function PlaceableItemProxy:_create_unit_info()
   local full_sized_info = self._full_sized_entity:get_component('unit_info')
   local unit_info_component = self._entity:add_component('unit_info')
   self._display_name = full_sized_info:get_display_name()
   unit_info_component:set_display_name(self._display_name)
   unit_info_component:set_description(full_sized_info:get_description())
   unit_info_component:set_icon(full_sized_info:get_icon())
   --TODO: faction??? Take care of this centrally?
end

--Copy item properties from the big entity into the proxy
function PlaceableItemProxy:_create_item_data()
   local full_sized_item_info = self._full_sized_entity:get_component('item')
   local item_component = self._entity:add_component('item')
   item_component:set_stacks(full_sized_item_info:get_stacks())
   item_component:set_max_stacks(full_sized_item_info:get_max_stacks())
   item_component:set_material(full_sized_item_info:get_material())
   item_component:set_category(full_sized_item_info:get_category())
   item_component:set_identifier(full_sized_item_info:get_identifier())
end



return PlaceableItemProxy