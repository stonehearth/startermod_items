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
      self._sv.citizen_scores = {}
      self._sv.notifications = {}
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
   local prefixes = self._data.town_pieces.optional_prefix
   local base_names = self._data.town_pieces.town_name
   local suffix = self._data.town_pieces.suffix

   --make a composite
   local target_prefix = prefixes[rng:get_int(1, #prefixes)]
   local target_base = base_names[rng:get_int(1, #base_names)]
   local target_suffix = suffix[rng:get_int(1, #suffix)]

   local composite_name = 'Defaultville'
   if target_base then
      composite_name = target_base
   end

   if target_prefix and rng:get_int(1, 100) < 40 then
      composite_name = target_prefix .. ' ' .. composite_name
   end

   if target_suffix and rng:get_int(1, 100) < 80 then
      composite_name = composite_name .. target_suffix
   end
   
   return composite_name
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

   self._sv.citizens[citizen:get_id()] = citizen
   stonehearth.score:update_aggregate_score(self._sv.player_id)

   self.__saved_variables:mark_changed()

   radiant.events.listen(citizen, 'radiant:entity:pre_destroy', self, self._on_entity_destroyed)

   return citizen
end

--Will show a simple notification that zooms to a citizen when clicked. 
--will expire if the citizen isn't around anymore
function PopulationFaction:show_notification_for_citizen(citizen, title)
   local citizen_id = citizen:get_id()
   if not self._sv.notifications[citizen_id] then
      self._sv.notifications[citizen_id] = {}
   elseif self._sv.notifications[citizen_id][title] then
      --If a bulletin already exists for this citizen with this title, remove it to replace with the new one
      local bulletin_id = self._sv.notifications[citizen_id][title]:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
   end
   self._sv.notifications[citizen_id][title] = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
            :set_callback_instance(self)
            :set_data({
               title = title,
               message = '',
               zoom_to_entity = citizen,
           })
end

function PopulationFaction:_on_entity_destroyed(args)  
   self._sv.citizens[args.entity_id] = nil
   stonehearth.score:update_aggregate_score(self._sv.player_id)

   --remove associated bulletins
   if self._sv.notifications[args.entity_id] then
      for title, bulletin in pairs(self._sv.notifications[args.entity_id]) do
         local bulletin_id = bulletin:get_id()
         stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      end
   end
   
   self.__saved_variables:mark_changed()
   return radiant.events.UNLISTEN
end

function PopulationFaction:customize_citizen(entity, all_variants, this_variant)   
   local variant = all_variants[this_variant]

   if not variant then
      return
   end
   
   -- load any models at this node in the customization tree
   if variant.models then
      local variant_name = 'default'
      local random_model = variant.models[rng:get_int(1, #variant.models)]
      local model_variants_component = entity:add_component('model_variants')
      model_variants_component:add_variant(variant_name):add_model(random_model)
   end

   -- for each set of child variants, pick a random option
   if variant.variants then
      for _, variant_set in ipairs(variant.variants) do
         local random_option = variant_set[rng:get_int(1, #variant_set)]
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

function PopulationFaction:promote_citizen(citizen, profession, talisman)
   citizen:add_component('stonehearth:profession'):promote_to(profession, talisman)
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

--- Given an entity, iterate through the array of people in this town and find the
--  person closest to the entity.
--  Returns the closest person and the entity's distance to that person. 
function PopulationFaction:find_closest_townsperson_to(entity)
   local shortest_distance = nil
   local closest_person = nil
   for id, citizen in pairs(self._sv.citizens) do
      if citizen:is_valid() and entity:get_id() ~= id then
         local distance = radiant.entities.distance_between(entity, citizen)
         if not shortest_distance or distance < shortest_distance then
            shortest_distance = distance
            closest_person = citizen
         end 
      end
   end
   return closest_person, shortest_distance
end

return PopulationFaction
