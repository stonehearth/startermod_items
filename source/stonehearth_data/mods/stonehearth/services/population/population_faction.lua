local PopulationFaction = class()

function PopulationFaction:__init(faction)
   self._faction = faction
   self._data = radiant.resources.load_json(faction)
   self._faction_name = 'civ' -- xxx: for now....
end

function PopulationFaction:create_new_citizen()   
   local gender = 'male'
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
