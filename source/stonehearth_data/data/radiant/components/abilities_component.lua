local Abilities = class()

function Abilities:__init(entity)
   self._entity = entity   
   self._abilities = {}
end
   
function Abilities:extend(json)
   -- not really...
   if json.ability_list then
      for name, uri in radiant.resources.pairs(json.ability_list) do
         self._abilities[name] = uri
      end
   end
end

function Abilities:__tojson()
   return radiant.json.encode(self._abilities)
end

function Abilities:do_ability(name, ...)
   local ability_uri = self._abilities[name]
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
