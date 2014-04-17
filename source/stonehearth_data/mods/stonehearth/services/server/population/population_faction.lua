local PopulationFaction = class()

local rng = _radiant.csg.get_default_rng()
local personality_service = stonehearth.personality

function PopulationFaction:__init(session, saved_variables)
   self.__saved_variables = saved_variables
   self._sv = self.__saved_variables:get_data()
   if session then
      self._sv.faction = session.faction
      self._sv.kingdom = session.kingdom
      self._sv.player_id = session.player_id
      self._sv.citizens = {}
   end
   self._data = radiant.resources.load_json(self._sv.kingdom)
end

function PopulationFaction:get_datastore(reason)
   return self.__saved_variables
end

function PopulationFaction:get_faction()
   return self._sv.faction
end

function PopulationFaction:get_player_id()
   return self._sv.player_id
end

function PopulationFaction:create_town_name()
   local town_names = self._data.town_names
   
   --TODO: make this a composite
   local this_name = 'Defaultville'
   if town_names then
      this_name = town_names[rng:get_int(1, #town_names)]
   end
   
   return this_name
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
   
   local all_variants = radiant.entities.get_entity_data(citizen, 'stonehearth:customization_variants')
   if all_variants then
      self:customize_citizen(citizen, all_variants, "root")
   end

   local unit_info = citizen:add_component('unit_info')
   unit_info:set_faction(self._sv.faction)
   unit_info:set_kingdom(self._sv.kingdom)
   unit_info:set_player_id(self._sv.player_id)

   self:_set_citizen_initial_state(citizen, gender)

   table.insert(self._sv.citizens, citizen)
   self.__saved_variables:mark_changed()

   return citizen
end

function PopulationFaction:customize_citizen(entity, all_variants, this_variant)   
   
   local variant = all_variants[this_variant]

   if not variant then
      return
   end
   
   -- load any models at this node in the customization tree
   if variant.models then
      local variant_name = 'default'
      local random_model = variant.models[1]
      local model_variants_component = entity:add_component('model_variants')
      model_variants_component:add_variant(variant_name):add_model(random_model)
   end

   -- for each set of child variants, pick a random option
   if variant.variants then
      for _, variant_set in ipairs(variant.variants) do
         local random_option = variant_set[math.random(#variant_set)]
         self:customize_citizen(entity, all_variants, random_option)
      end
   end
end

function PopulationFaction:get_citizens()
   return self._sv.citizens
end

function PopulationFaction:_set_citizen_initial_state(citizen, gender)
   -- name
   local name = self:generate_random_name(gender)
   radiant.entities.set_display_name(citizen, name)

   -- personality
   local personality = personality_service:get_new_personality()
   local personality_component = citizen:add_component('stonehearth:personality')
   personality_component:set_personality(personality)

   --For the teacher field, assign the one appropriate for this kingdom
   personality_component:add_substitution_by_parameter('teacher', self._sv.kingdom, 'stonehearth')
   personality_component:add_substitution_by_parameter('personality_based_exclamation', personality, 'stonehearth:settler_journals')

end

function PopulationFaction:create_entity(uri)
   local entity = radiant.entities.create_entity(uri)
   entity:add_component('unit_info'):set_faction(self._sv.faction)
   return entity
end

function PopulationFaction:promote_citizen(citizen, profession)
   citizen:add_component('stonehearth:profession'):promote_to(profession)
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
