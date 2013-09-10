--[[
   stonehearth_names
   This calls loads in names from various locations and puts them in arrays.

   For now, the structure of the json looks like this:
   {
      faction_name: "foo"
      faction_id : "id"
      surnames: [
      ],
      given_names: {
         male: [
         ],
         female: [
         ]
      }
   }
   And all json is turned into objects and indexed via self._names[faction_id]

   --TODO: better as a database? How to make this extensible?
]]

local NameGenerator = class()

function NameGenerator:__init()
   self._names = {}
   --Read in all the factions
   --TODO: put this in a manifest? I mean, it's inside the same module...
   local name_index = radiant.resources.load_json('/stonehearth_names/data/name_index.json')
   local table_len = radiant.resources.len(name_index.factions)
   for i = 1, table_len do
      local faction_data = radiant.resources.load_json(name_index.factions[i])
      self._names[faction_data.faction_id] = faction_data
   end
end

--[[
   returns: A first and last name as one string
]]
function NameGenerator:get_random_name(faction_id, gender)
   local faction_data = self._names[faction_id]

   if not faction_data then
      return 'Faction NotFound'
   end

   local random_surname = self:_pick_random_from_array(faction_data.surnames)
   local random_first = 'default'
   if gender == 'female' then
      random_first = self:_pick_random_from_array(faction_data.given_names.female)
   else
      random_first = self:_pick_random_from_array(faction_data.given_names.male)
   end
   return random_first .. ' ' .. random_surname
end

function NameGenerator:_pick_random_from_array(array)
   local numContents = radiant.resources.len(array)
   local random_index = math.random(numContents)
   return array[random_index]
end

return NameGenerator