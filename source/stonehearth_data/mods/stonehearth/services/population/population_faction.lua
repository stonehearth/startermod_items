local PopulationFaction = class()

local personality_service = require 'services.personality.personality_service'

--Separate the faction name (player chosen) from the kingdom name (ascendency, etc.)
function PopulationFaction:__init(faction, kingdom)
   self._faction = faction
   self._data = radiant.resources.load_json(kingdom)
   self._faction_name = faction --TODO: differentiate b/w user id and name?
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

   citizen:add_component('unit_info'):set_faction(self._faction_name) -- xxx: for now...

   self:_set_citizen_initial_state(citizen, gender)
   return citizen
end

function PopulationFaction:_set_citizen_initial_state(citizen, gender)
   -- name
   local name = self:generate_random_name(gender)
   radiant.entities.set_display_name(citizen, name)

   -- pesonality
   local personality = personality_service:get_new_personality()
   citizen:add_component('stonehearth:personality'):set_personality(personality)

   -- stats 
   radiant.entities.set_attribute(citizen, 'mind', math.random(1, 6))
   radiant.entities.set_attribute(citizen, 'body', math.random(1, 6))
   radiant.entities.set_attribute(citizen, 'spirit', math.random(1, 6))

   radiant.entities.set_attribute(citizen, 'diligence', 100)
   radiant.entities.set_attribute(citizen, 'curiosity', 100)
   radiant.entities.set_attribute(citizen, 'inventiveness', 100)


   radiant.entities.set_attribute(citizen, 'muscle', 100)
   radiant.entities.set_attribute(citizen, 'stamina', 100)


   radiant.entities.set_attribute(citizen, 'courage', 100)
   radiant.entities.set_attribute(citizen, 'willpower', 100)
   radiant.entities.set_attribute(citizen, 'compassion', 100)

   -- randomize hunger
   radiant.entities.set_attribute(citizen, 'hunger', math.random(1, 30))

   -- randomize sleepiness
   radiant.entities.set_attribute(citizen, 'sleepiness', math.random(1, 30))
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
