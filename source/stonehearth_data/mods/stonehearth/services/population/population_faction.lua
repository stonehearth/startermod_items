local PopulationFaction = class()

--Separate the faction name (player chosen) from the kingdom name (ascendency, etc.)
function PopulationFaction:__init(faction, kingdom)
   self._faction = faction
   self._data = radiant.resources.load_json(kingdom)
   self._faction_name = faction --TODO: differentiat b/w user id and name?
end

function PopulationFaction:create_new_citizen()   
   local gender

   -- xxx, replace this with a coin flip using math.random
   if not self._always_one_girl_hack then
      gender = 'female'
      self._always_one_girl_hack = true
   else 
      if math.random(2) == 1 then
         gender = 'male'
      else 
         gender = 'female'
      end
   end

   local entities = self._data[gender .. '_entities']
   local kind = entities[math.random(#entities)]
   local citizen = radiant.entities.create_entity(kind)
   local name = self:generate_random_name(gender)
   radiant.entities.set_display_name(citizen, name)
   
   citizen:add_component('unit_info'):set_faction(self._faction_name) -- xxx: for now...
   return citizen
end

function PopulationFaction:create_entity(uri)
   local entity = radiant.entities.create_entity(uri)
   entity:add_component('unit_info'):set_faction(self._faction_name)
   return entity
end

function PopulationFaction:promote_citizen(citizen, profession)
   local profession_api = radiant.mods.require(string.format('stonehearth.professions.%s.%s', profession, profession))
   profession_api.promote(citizen)
end

function PopulationFaction:get_home_location()
   return self._town_location
end

function PopulationFaction:set_home_location(location)
   self._town_location = location
end

function PopulationFaction:generate_random_name(gender)
   local first_names
   if gender == 'female' then
      first_names = self._data.given_names.female
   else
      first_names = self._data.given_names.male
   end
   local first = first_names[math.random(#first_names)]
   local surname = self._data.surnames[math.random(#self._data.surnames)]

   return first .. ' ' .. surname
end

return PopulationFaction
