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
   And all json is turned into objects and indexed via singleton._names[faction_id]

   --TODO: better as a database? How to make this extensible?
]]

local singleton = {}
local name_generator = {}

function name_generator._init()
   singleton._names = {}
   --Read in all the factions
   --TODO: put this in a manifest? I mean, it's inside the same module...

   local json = radiant.resources.load_json('stonehearth_names/data/name_index.json')
   if json and json.factions then
      for i, faction_file in ipairs(json.factions) do
         local faction_data = radiant.resources.load_json(faction_file)
         if faction_data then
            singleton._names[faction_data.faction_id] = faction_data
         end
      end
   end
end

--[[
   returns: A first and last name as one string
]]
function name_generator.generate_random_name(faction_id, gender)
   local faction_data = singleton._names[faction_id]

   if not faction_data then
      return 'Faction NotFound'
   end

   local first_names = gender == 'female' and faction_data.given_names.female or faction_data.given_names.male
   local random_first = first_names[math.random(#first_names)]
   local random_surname = faction_data.surnames[math.random(#faction_data.surnames)]
   return random_first .. ' ' .. random_surname
end

name_generator._init()

return name_generator
