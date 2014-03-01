--[[
   Use this component to denote an entity that the player has
   placed in the world, but isn't actully yet constructed or placed
   by the people in the world.
   TODO: Only show these items in a special mode (build mode)
]]

local GhostItemComponent = class()

function GhostItemComponent:__create(entity, json)
   self._entity = entity

   self._data {
      full_sized_mod_url = ''
      unit_info_name = 'Ghooooost Item'
      unit_info_description = 'Whoooo aaaammmm Iiiiiiii?'
      unit_info_icon = ''
   }

   self.__savestate = radiant.create_datastore(self._data)
end

function GhostItemComponent:set_full_sized_mod_uri(real_item_uri)
   self._data.full_sized_mod_url = real_item_uri

   local json = radiant.resources.load_json(real_item_uri)
   if json and json.components then
      if json.components.unit_info then
         local data = json.components.unit_info

         --TODO: double check with Tony as to why this works
         self._data.unit_info_name = data.name and data.name or ''
         self._data.unit_info_description = data.description and data.description or ''
         self._data.unit_info_icon = data.icon and data.icon or ''
      end
   end
   self._data_binding:mark_changed()
end

function GhostItemComponent:get_full_sized_mod_uri()
   return self._data.full_sized_mod_url
end

function GhostItemComponent:get_full_sized_json()
   return radiant.resources.load_json(self._data.full_sized_mod_url)
end

return GhostItemComponent