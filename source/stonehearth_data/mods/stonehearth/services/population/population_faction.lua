local PopulationFaction = class()

local personality_service = require 'services.personality.personality_service'
local rng = _radiant.csg.get_default_random_number_generator()

--Separate the faction name (player chosen) from the kingdom name (ascendency, etc.)
function PopulationFaction:__init(faction, kingdom)
   self._faction = faction
   self._data = radiant.resources.load_json(kingdom)
   self._faction_name = faction --TODO: differentiate b/w user id and name?
   self._kingdom = self._data.kingdom_id
end

function PopulationFaction:create_new_citizen()   
   local gender

   -- xxx, replace this with a coin flip using rng
   if not self._always_one_girl_hack then
      gender = 'female'
      self._always_one_girl_hack = true
   else 
      if rng:get_int(1, 2) == 1 then
         gender = 'male'
      else 
         gender = 'female'
      end
   end

   local entities = self._data[gender .. '_entities']
   local kind = entities[rng:get_int(1, #entities)]
   local citizen = radiant.entities.create_entity(kind)

   citizen:add_component('unit_info'):set_faction(self._faction_name) -- xxx: for now...
   --citizen:add_component('unit_info'):set_kingdom(self.kingdom)

   self:_set_citizen_initial_state(citizen, gender)
   return citizen
end

function PopulationFaction:_set_citizen_initial_state(citizen, gender)
   -- name
   local name = self:generate_random_name(gender)
   radiant.entities.set_display_name(citizen, name)

   -- pesonality
   local personality = personality_service:get_new_personality()
   local personality_component = citizen:add_component('stonehearth:personality')
   personality_component:set_personality(personality)
   --For the deity field, assign the one appropriate for this kingdom
   personality_component:add_substitution_by_parameter('deity', self._kingdom)

   local mind = rng:get_int(1, 6)
   local body = rng:get_int(1, 6)
   local spirit = rng:get_int(1, 6)
   -- stats 
   radiant.entities.set_attribute(citizen, 'mind', mind)
   radiant.entities.set_attribute(citizen, 'body', body)
   radiant.entities.set_attribute(citizen, 'spirit', spirit)

   radiant.entities.set_attribute(citizen, 'diligence', self:get_derived_attribute(mind))
   radiant.entities.set_attribute(citizen, 'curiosity', self:get_derived_attribute(mind))
   radiant.entities.set_attribute(citizen, 'inventiveness', self:get_derived_attribute(mind))


   radiant.entities.set_attribute(citizen, 'muscle', self:get_derived_attribute(body))
   radiant.entities.set_attribute(citizen, 'speed', self:get_derived_attribute(body))
   radiant.entities.set_attribute(citizen, 'stamina', self:get_derived_attribute(body))


   radiant.entities.set_attribute(citizen, 'courage', self:get_derived_attribute(spirit))
   radiant.entities.set_attribute(citizen, 'willpower', self:get_derived_attribute(spirit))
   radiant.entities.set_attribute(citizen, 'compassion', self:get_derived_attribute(spirit))

   -- speed is a factor of body as well
   local speed = math.floor(80 + (20 * body/6))
   radiant.entities.set_attribute(citizen, 'speed', speed)

   -- randomize hunger
   radiant.entities.set_attribute(citizen, 'hunger', rng:get_int(1, 30))

   -- randomize sleepiness
   radiant.entities.set_attribute(citizen, 'sleepiness', rng:get_int(1, 30))
end

function PopulationFaction:get_derived_attribute(primary_attribute_value)
   local min = primary_attribute_value * 10
   local max = min + 9
   return rng:get_int(min, max)
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

   local first = first_names[rng:get_int(1, #first_names)]
   local surname = self._data.surnames[rng:get_int(1, #self._data.surnames)]

   return first .. ' ' .. surname
end

return PopulationFaction
