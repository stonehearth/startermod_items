-- Service that ticks once per hour to decay food.
local rng = _radiant.csg.get_default_rng()

FoodDecayService = class()

function FoodDecayService:initialize()
   self.food_type_tags = {"raw_food", "prepared_food", "luxury_food"}

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv._decaying_food = {}
      self._sv.decay_listener = stonehearth.calendar:set_interval("FoodDecayService on_decay", '1h', function()
            self:_on_decay()
         end)
      self._sv.food_type_counts = {}
   else
      self._sv.decay_listener:bind(function() 
         self:_on_decay()
      end)
   end

   local entity_container = radiant._root_entity:add_component('entity_container')
   self._entity_container_trace = entity_container:trace_children('FoodDecayService food in world')
      :on_added(function(id, entity)
            self:_on_entity_added_to_world(entity)
         end)

   radiant.events.listen(radiant, 'radiant:entity:post_destroy', function(e)
         local entity_id = e.entity_id
         self:_on_entity_destroyed(entity_id)
      end)
end

function FoodDecayService:get_raw_food_count()
   return self._sv.food_type_counts[self.food_type_tags[1]] or 0
end

function FoodDecayService:get_prepared_food_count()
   return self._sv.food_type_counts[self.food_type_tags[2]] or 0
end

function FoodDecayService:get_luxury_food_count()
   return self._sv.food_type_counts[self.food_type_tags[3]] or 0
end

function FoodDecayService:_on_decay()
   for _, food_decay_data in pairs(self._sv._decaying_food) do
      local entity = food_decay_data.entity
      local food_container = radiant.entities.get_entity_data(entity, 'stonehearth:food_container')
      local decay_tuning = food_container.decay
      local initial_health = radiant.entities.get_attribute(entity, 'health')
      local decayed_value = rng:get_int(decay_tuning.health_per_hour.min, decay_tuning.health_per_hour.max)
      local health = initial_health - decayed_value;
      radiant.entities.set_attribute(entity, 'health', health)
      if health <= 0 then
         -- Food is rotten beyond recognition. Destroy it.
         radiant.entities.destroy_entity(entity)
      else
         -- Figure out if we're supposed to be in some other model state
         local unit_info = entity:get_component('unit_info')

         local model = 'default'
         local new_description = unit_info:get_description()
         local lowest_trigger_value = initial_health + 1
         for _, decay_stage in pairs(decay_tuning.decay_stages) do
            -- Find the decay stage most suited for our health value.
            -- Unfortunately this means iterating through all the stages,
            -- but there should only be 2 stages or so.
            if health <= decay_stage.trigger_health_value and decay_stage.trigger_health_value < lowest_trigger_value then
               lowest_trigger_value = decay_stage.trigger_health_value
               model = decay_stage.model_variant
               new_description = decay_stage.description
            end
         end

         unit_info:set_description(new_description)
         entity:get_component('render_info'):set_model_variant(model)
      end
   end
end

function FoodDecayService:_get_food_type(entity)
   for _, food_type in ipairs(self.food_type_tags) do
      if radiant.entities.is_material(entity, food_type) then
         return food_type
      end
   end
   return 'unknown'
end

function FoodDecayService:_on_entity_added_to_world(entity)
   local food_container = radiant.entities.get_entity_data(entity, 'stonehearth:food_container')
   if food_container and food_container.decay then
      local food_type = self:_get_food_type(entity)
      self._sv._decaying_food[entity:get_id()] = { entity = entity, food_type = food_type }

      local count = self._sv.food_type_counts[food_type] or 0
      count = count + 1
      self._sv.food_type_counts[food_type] = count
   end
end

function FoodDecayService:_on_entity_destroyed(entity_id)
   local decay_data = self._sv._decaying_food[entity_id]
   if decay_data then -- If what's being destroyed is a food
      local food_type = decay_data.food_type
      local count = self._sv.food_type_counts[food_type]
      if count then
         count = count - 1
         self._sv.food_type_counts[food_type] = count
      end
      self._sv._decaying_food[entity_id] = nil
   end
end

return FoodDecayService