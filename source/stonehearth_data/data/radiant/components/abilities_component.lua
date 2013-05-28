local Abilities = class()

function Abilities:__init(entity, json)
   self._entity = entity
   self._abilities = json.ability_list
end

function Abilities:do_ability(name, ...)
   local ability_uri = self._abilities[name];
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
