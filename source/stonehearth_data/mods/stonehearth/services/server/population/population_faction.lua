local PopulationFaction = class()

local rng = _radiant.csg.get_default_rng()
local personality_service = stonehearth.personality

function PopulationFaction:__init(player_id, kingdom, saved_variables)
   self.__saved_variables = saved_variables
   self._sv = self.__saved_variables:get_data()
   self._log = radiant.log.create_logger('population')
   if player_id then
      self._sv.kingdom = kingdom
      self._sv.player_id = player_id
      self._sv.citizens = {}
      self._sv.bulletins = {}
      self._sv._global_vision = {}
      self._sv.is_npc = true
      self._sv.threat_level = 0
   end

   self._sensor_traces = {}
   self._data = radiant.resources.load_json(self._sv.kingdom)

   for id, citizen in pairs(self._sv.citizens) do
      self:_monitor_citizen(citizen)
   end
end

function PopulationFaction:get_datastore(reason)
   return self.__saved_variables
end

function PopulationFaction:get_kingdom()
   return self._sv.kingdom
end

function PopulationFaction:get_player_id()
   return self._sv.player_id
end

function PopulationFaction:is_npc()
   return self._sv.is_npc
end

function PopulationFaction:set_is_npc(value)
   self._sv.is_npc = value
   self.__saved_variables:mark_changed()
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

function PopulationFaction:create_new_citizen(role)
   local gender
   checks("self", "?string")

   if not role then
      role = "default"
   end

   if radiant.empty(self._sv.citizens) then
      gender = 'female'
   else 
      if rng:get_int(1, 2) == 1 then
         gender = 'male'
      else 
         gender = 'female'
      end
   end

   local role_data = self._data.roles[role]
   if not role_data then
      error(string.format('unknown role %s in population', role))
   end
   
   --If there is no gender, default to male
   if not role_data[gender] then
      gender = 'male'
   end
   local entities = role_data[gender].uri
   if not entities then
      error(string.format('role %s in population has no gender table for %s', role, gender))
   end

   local kind = entities[rng:get_int(1, #entities)]
   local citizen = radiant.entities.create_entity(kind, { owner = self._sv.player_id })
   
   local all_variants = radiant.entities.get_entity_data(citizen, 'stonehearth:customization_variants')
   if all_variants then
      self:customize_citizen(citizen, all_variants, "root")
   end

   citizen:add_component('unit_info')
               :set_player_id(self._sv.player_id)

   self:_set_citizen_initial_state(citizen, gender, role_data)

   self._sv.citizens[citizen:get_id()] = citizen
   stonehearth.score:update_aggregate_score(self._sv.player_id)

   self.__saved_variables:mark_changed()

   self:_monitor_citizen(citizen)

   return citizen
end

function PopulationFaction:_monitor_citizen(citizen)
   local citizen_id = citizen:get_id()

   -- listen for entity destroy bulletins so we'll know when the pass away
   radiant.events.listen(citizen, 'radiant:entity:pre_destroy', self, self._on_entity_destroyed)

   -- subscribe to their sensor so we can look for trouble.
   local sensor_list = citizen:get_component('sensor_list')
   if sensor_list then
      local sensor = sensor_list:get_sensor('sight')
      if sensor then
         self._sensor_traces[citizen_id] = sensor:trace_contents('monitoring threat level')
                                                      :on_added(function(visitor_id, visitor)
                                                            self:_on_seen_by(citizen_id, visitor_id, visitor)
                                                         end)
                                                      :on_removed(function(visitor_id)
                                                            self:_on_unseen_by(citizen_id, visitor_id)
                                                         end)
                                                      :push_object_state()

      end
   end   
end

function PopulationFaction:_get_threat_level(visitor)
   if radiant.entities.is_friendly(self._sv.player_id, visitor) then
      return 0
   end
   return radiant.entities.get_attribute(visitor, 'menace', 0)
end

function PopulationFaction:_on_seen_by(spotter_id, visitor_id, visitor)
   local threat_level = self:_get_threat_level(visitor)
   if threat_level <= 0 then
      -- not interesting.  move along!
      return
   end

   local entry = self._sv._global_vision[visitor_id]
   if not entry then
      entry = {
         seen_by = { [spotter_id] = true },
         threat_level = threat_level,
         entity = visitor,
      }
      self._sv._global_vision[visitor_id] = entry

      self:_update_threat_level()

      radiant.events.trigger_async(self, 'stonehearth:population:new_threat', {
            entity_id = visitor_id,
            entity = visitor,
         });     
   end 
   entry.seen_by[spotter_id] = true
end

function PopulationFaction:_on_unseen_by(spotter_id, visitor_id)
   local entry = self._sv._global_vision[visitor_id]
   if entry then
      entry.seen_by[spotter_id] = nil
      self._log:debug("visitor %d still seen by %d citizens", visitor_id, radiant.size(entry.seen_by))
      if radiant.empty(entry.seen_by) then
         self._sv._global_vision[visitor_id] = nil
         self:_update_threat_level()
      end
   end
end

--Will show a simple notification that zooms to a citizen when clicked. 
--will expire if the citizen isn't around anymore
function PopulationFaction:show_notification_for_citizen(citizen, title)
   local citizen_id = citizen:get_id()
   if not self._sv.bulletins[citizen_id] then
      self._sv.bulletins[citizen_id] = {}
   elseif self._sv.bulletins[citizen_id][title] then
      --If a bulletin already exists for this citizen with this title, remove it to replace with the new one
      local bulletin_id = self._sv.bulletins[citizen_id][title]:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
   end
   self._sv.bulletins[citizen_id][title] = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
            :set_callback_instance(self)
            :set_data({
               title = title,
               message = '',
               zoom_to_entity = citizen,
           })
end

function PopulationFaction:_on_entity_destroyed(evt)
   local entity_id = evt.entity_id

   -- update the score
   if self._sv.citizens[entity_id] then
      self:_on_citizen_destroyed(entity_id)
   end
   if self._sv._global_vision[entity_id] then
      self:_on_global_vision_entity_destroyed(evt.entity_id)
   end
end

function PopulationFaction:_on_citizen_destroyed(entity_id)
   self._sv.citizens[entity_id] = nil
   stonehearth.score:update_aggregate_score(self._sv.player_id)

   -- remove associated bulletins
   local bulletins = self._sv.bulletins[entity_id]
   if bulletins then
      self._sv.bulletins[entity_id] = nil
      for title, bulletin in pairs(bulletins) do
         local bulletin_id = bulletin:get_id()
         stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      end
   end   

   -- nuke sensors
   local sensor_trace = self._sensor_traces[entity_id]
   if sensor_trace then
      self._sensor_traces[entity_id] = nil
      sensor_trace:destroy()
   end

   -- global vision
   for visitor_id, _ in pairs(self._sv._global_vision) do
      self:_on_unseen_by(entity_id, visitor_id)
   end

   self.__saved_variables:mark_changed()
   return radiant.events.UNLISTEN
end

function PopulationFaction:_on_global_vision_entity_destroyed(entity_id)
   self._sv._global_vision[entity_id] = nil
   self:_update_threat_level()
end

function PopulationFaction:_update_threat_level()
   local threat_level = 0
   for _, entry in pairs(self._sv._global_vision) do
      threat_level = threat_level + entry.threat_level
   end
   self._sv.threat_level = threat_level
   self.__saved_variables:mark_changed()
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

function PopulationFaction:_set_citizen_initial_state(citizen, gender, role_data)
   -- name
   local name = self:generate_random_name(gender, role_data)
   if name then
      radiant.entities.set_display_name(citizen, name)
   end
   
   -- personality
   --TODO: parametrize these by role too?
   local personality = personality_service:get_new_personality()
   local personality_component = citizen:add_component('stonehearth:personality')
   personality_component:set_personality(personality)

   --For the teacher field, assign the one appropriate for this kingdom
   personality_component:add_substitution_by_parameter('teacher', self._sv.kingdom, 'stonehearth')
   personality_component:add_substitution_by_parameter('personality_based_exclamation', personality, 'stonehearth:settler_journals')

end

function PopulationFaction:create_entity(uri)
   return radiant.entities.create_entity(uri, { owner = self._sv.player_id })
end

function PopulationFaction:get_home_location()
   return self._town_location
end

function PopulationFaction:set_home_location(location)
   self._town_location = location
end

function PopulationFaction:generate_random_name(gender, role_data)
   if role_data[gender].given_names then
      local first_names = ""

      first_names = role_data[gender].given_names

      local first = first_names[rng:get_int(1, #first_names)]
      local surname = ""
      if role_data.surnames then
         surname = role_data.surnames[rng:get_int(1, #role_data.surnames)]
      end
      return first .. ' ' .. surname
   else
      return nil
   end
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
 