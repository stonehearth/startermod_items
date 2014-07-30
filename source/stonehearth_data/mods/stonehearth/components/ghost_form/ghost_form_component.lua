--[[
   Use this component to denote an entity that the player has
   placed in the world, but isn't actully yet constructed or placed
   by the people in the world.
   TODO: Only show these items in a special mode (build mode)
]]

local GhostFormComponent = class()

function GhostFormComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.full_sized_mod_url then
      self._sv.full_sized_mod_url = ''
      self._sv.unit_info_name = 'Ghooooost Item'
      self._sv.unit_info_description = 'Whoooo aaaammmm Iiiiiiii?'
      self._sv.unit_info_icon = ''
   end
end

function GhostFormComponent:set_full_sized_mod_uri(real_item_uri)
   self._sv.full_sized_mod_url = real_item_uri

   local json = radiant.resources.load_json(real_item_uri)
   if json and json.components then
      if json.components.unit_info then
         local data = json.components.unit_info

         --TODO: double check with Tony as to why this works
         self._sv.unit_info_name = data.name and data.name or ''
         self._sv.unit_info_description = data.description and data.description or ''
         self._sv.unit_info_icon = data.icon and data.icon or ''
      end
   end
   self.__saved_variables:mark_changed()
end

function GhostFormComponent:get_full_sized_mod_uri()
   return self._sv.full_sized_mod_url
end

function GhostFormComponent:get_full_sized_json()
   return radiant.resources.load_json(self._sv.full_sized_mod_url)
end

return GhostFormComponent
