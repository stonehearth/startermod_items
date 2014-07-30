--[[
   Placeable_item_proxy.lua
   Some items are created in the world and immediately exist.
   Other items are created as "icons" and don't have the properties of
   their final state until they are placed in the world by the player.
   This component represents one of those items before it is placed.
   It contains a link to the real-world entity it is linked to.
]]

local PlaceableItemProxyComponent = class()

function PlaceableItemProxyComponent:initialize(entity, json)
   self._entity = entity               --The 1x1 placeable cube
   self._placed_entity_uri = json.full_sized_entity
   self._hide_place_item_command = json.hide_place_item_command

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.category = json.category or 'uncategorized'
      self._sv.placeable_on_ground = json.placeable_on_ground  or false
      self._sv.placeable_on_walls = json.placeable_on_walls or false
      self:_create_derived_components()
      self.__saved_variables:mark_changed()
   end
end

function PlaceableItemProxyComponent:get_category()
   return self._sv.category
end

-- Implemented in such a way that one can query a proxy without having to
-- actually create the full sized entity (if it hasn't been created yet).
function PlaceableItemProxyComponent:get_full_sized_entity_uri()
   assert(self._placed_entity_uri)
   return self._placed_entity_uri
end

function PlaceableItemProxyComponent:get_full_sized_entity()
   if not self._sv.placed_entity and self._placed_entity_uri then
      local placed_entity = radiant.entities.create_entity(self._placed_entity_uri)
      self:set_full_sized_entity(placed_entity)
   end
   return self._sv.placed_entity
end

--- If something started as a big entity and then we have to carry it
-- save it's full-sized entity here, to preserve its values
function PlaceableItemProxyComponent:set_full_sized_entity(placed_entity)
   local self_placed_entity = self._sv.placed_entity
   assert(not self_placed_entity or self_placed_entity:get_id() == placed_entity:get_id())

   if self_placed_entity ~= nil then
      return
   end

   self._sv.placed_entity = placed_entity
   self._placed_entity_uri = placed_entity:get_uri()
   --TODO: clarify: why might we be missing this URI?
   if not self._placed_entity_uri then
      self._placed_entity_uri = placed_entity:get_uri()
      self:_create_derived_components()
   end

   --Copy the unit info data to the new object
   --TODO: if we need to do this often, move to radiant.entities?
   local unit_info_component = self._sv.placed_entity:add_component('unit_info')
   unit_info_component:set_player_id(radiant.entities.get_player_id(self._entity))
   unit_info_component:set_faction(radiant.entities.get_faction(self._entity))
   unit_info_component:set_kingdom(radiant.entities.get_kingdom(self._entity))


   self.__saved_variables:mark_changed()

   placed_entity:add_component('stonehearth:placed_item')   
                         :set_proxy_entity(self._entity)

end

--Copy the unit info from the big entity's json file into the proxy
function PlaceableItemProxyComponent:_create_derived_components()
   if not self._placed_entity_uri then
      return
   end

   local clone_components = {
      'unit_info',
      'stonehearth:material'
   }

   local json = radiant.resources.load_json(self._placed_entity_uri)

   if json and json.components then
      for i, component in ipairs(clone_components) do
         if json.components[component] then
            self._entity:add_component(component, json.components[component])
         end
      end
   end

   --Issues: if this is in a parent class, it isn't loaded by this point, so add manually
   if not self._hide_place_item_command then
      local commands = self._entity:add_component('stonehearth:commands')
      commands:add_command('/stonehearth/data/commands/place_item')

      commands:modify_command('place_item', function (command_data)
         command_data.event_data.full_sized_entity_uri = self._placed_entity_uri
         command_data.event_data.proxy = self
         command_data.event_data.item_name = self._entity:add_component('unit_info'):get_display_name()
      end)
   end    
end

return PlaceableItemProxyComponent