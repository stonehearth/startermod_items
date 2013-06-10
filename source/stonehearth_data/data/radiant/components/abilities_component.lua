local Abilities = class()

function Abilities:__init(entity)
   self._entity = entity   
   self._data = {
      abilities = {}
   }   
   radiant.components.mark_dirty(self, self._data)
end
   
function Abilities:extend(json)
   -- not really...
   if json.ability_list then
      for name, uri in radiant.resources.pairs(json.ability_list) do
         self._data.abilities[name] = uri
      end
      radiant.components.mark_dirty(self, self._data)
   end
end

function Abilities:do_ability(name, ...)
   local ability_uri = self._data.abilities[name]
   if ability_uri then
      local ability_data = radiant.resources.load_json(ability_uri)
      if ability_data then
         local api = radiant.mods.require(ability_data.handler)
         if api and api[ability_data.command] then
            -- xxx: validate args...
            return api[ability_data.command](self._entity, ...)
         end
      end
   end
end

return Abilities
