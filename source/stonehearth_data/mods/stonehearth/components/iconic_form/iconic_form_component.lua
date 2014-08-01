--[[
   Some items are created in the world and immediately exist.
   Other items are created as "icons" and don't have the properties of
   their final state until they are placed in the world by the player.
   This component represents one of those items before it is placed.
   It contains a link to the real-world entity it is linked to.
]]

local UNIT_INFO_PROPERTIES = {
   'display_name',
   'description',
   'icon',
   'portrait'
}

local IconicFormComponent = class()

function IconicFormComponent:initialize(entity, json)
   self._entity = entity

   assert(not next(json), 'iconic_form components should not be specified in json files')
   if not self._sv.root_entity then
      self._entity:set_debug_text(self._entity:get_debug_text() .. ' (iconic)')
   end
end

function IconicFormComponent:get_root_entity()
   return self._sv.root_entity
end

function IconicFormComponent:set_root_entity(root_entity)
   assert(not self._sv.root_entity, 'root entity should be initialized exactly once')
   self._sv.root_entity = root_entity
   self.__saved_variables:mark_changed()

   -- copy the unit_info if we're missing any bits
   local unit_info = self._entity:add_component('unit_info')
   local root_unit_info = root_entity:add_component('unit_info')
   
   for _, name in ipairs(UNIT_INFO_PROPERTIES) do
      if #unit_info['get_'..name](unit_info) == 0 then
         local value = root_unit_info['get_'..name](root_unit_info)
         unit_info['set_'..name](unit_info, value)
      end
   end
   local root_material = root_entity:get_component('stonehearth:material')
   if root_material then
      self._entity:add_component('stonehearth:material')
                     :set_tags(root_material:get_tags())
   end
   return self
end

function IconicFormComponent:get_ghost_entity()
   return self._sv.ghost_entity
end

function IconicFormComponent:set_ghost_entity(ghost_entity)
   assert(not self._sv.ghost_entity, 'ghost entity should be initialized exactly once')
   self._sv.ghost_entity = ghost_entity
   self.__saved_variables:mark_changed()
   return self
end

function IconicFormComponent:set_placeable(placeable)
   if placeable and radiant.is_server then
      -- add the command to place the item from the proxy
      self._entity:add_component('stonehearth:commands')
                     :add_command('/stonehearth/data/commands/place_item')
   end
   return self
end

return IconicFormComponent
