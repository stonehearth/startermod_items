--[[
   Placeable_item_proxy.lua
   Some items are created in the world and immediately exist.
   Other items are created as "icons" and don't have the properties of
   their final state until they are placed in the world by the player.
   This component represents one of those items before it is placed.
   It contains a link to the real-world entity it is linked to.
]]

local PlaceableItemProxyComponent = class()

function PlaceableItemProxyComponent:__init(entity, data_binding)
   self._entity = entity               --The 1x1 placeable cube
   self._full_sized_entity = nil       --Onced initialized, the actual full-sized entity

   self._data = data_binding:get_data()
   self._data.entity_id = entity:get_id()

   self._under_construction = false

   self._data_binding = data_binding
   self._data_binding:mark_changed()
end

function PlaceableItemProxyComponent:extend(json)
   if json and json.full_sized_entity then
      self._data.full_sized_entity_uri = json.full_sized_entity;
      self._data_binding:mark_changed()

      self:_create_derived_components()
   end
end

function PlaceableItemProxyComponent:get_full_sized_entity()
   if not self._full_sized_entity then
      self:_create_full_sized_entity();
   end
   return self._full_sized_entity
end

--- If something started as a big entity and then we have to carry it
-- save it's full-sized entity here, to preserve its values
function PlaceableItemProxyComponent:set_full_sized_entity(full_sized_entity)
   self._full_sized_entity = full_sized_entity
end

--Get the uri of the full sized entity to the place command
function PlaceableItemProxyComponent:get_full_sized_entity_uri()
   return self._data.full_sized_entity_uri
end

--[[
   Create the entity and give it our faction
]]
function PlaceableItemProxyComponent:_create_full_sized_entity()
   self._full_sized_entity = radiant.entities.create_entity(self._data.full_sized_entity_uri)
   self._full_sized_entity:add_component('stonehearth:placed_item'):set_proxy(self._entity)

   local proxy_faction = radiant.entities.get_faction(self._entity)
   self._full_sized_entity:add_component('unit_info'):set_faction(proxy_faction)
end

--Copy the unit info from the big entity's json file into the proxy
function PlaceableItemProxyComponent:_create_derived_components()
   local full_sized_uri = self:get_full_sized_entity_uri()
   local display_name = ''

   local clone_components = {
      'unit_info',
      'stonehearth:material'
   }

   local json = radiant.resources.load_json(full_sized_uri)

   if json and json.components then
      for i, component in ipairs(clone_components) do
         if json.components[component] then
            local the_component = self._entity:add_component(component)
            the_component:extend(json.components[component])
         end
      end
   end
   --Issues: if this is in a parent class, it isn't loaded by this point, so add manually
   local commands = self._entity:add_component('stonehearth:commands')
   commands:add_command('/stonehearth/data/commands/place_item')

   commands:modify_command('place_item', function (command_data)
      command_data.event_data.full_sized_entity_uri = full_sized_uri
      command_data.event_data.proxy = self._data_binding
      command_data.event_data.item_name = self._entity:add_component('unit_info'):get_display_name()
   end)
end

return PlaceableItemProxyComponent