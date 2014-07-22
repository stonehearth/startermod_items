--[[
   Observes the HP of the entity and shows popup toasts based on their current HP
]]

local HealthObserver = class()

function HealthObserver:initialize(entity, json)
   self._entity = entity
   self._attributes_component = entity:add_component('stonehearth:attributes')

   --After everything has been initialized, then set the current 
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true

      --When everything has been init for the first time, store the max health
      --Listen on health changes only after we have a base health
      --radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         self._sv.last_health = self._attributes_component:get_attribute('max_health')
         self:_calculate_thresholds()
      --end)
   end

   radiant.events.listen(self._entity, 'stonehearth:attribute_changed:health', self, self._on_health_changed)
end

-- We care about full health, three quarters, half, and quarter health
-- Given full health, calculate the others
function HealthObserver:_calculate_thresholds()
   self._sv._max_health = self._sv.last_health
   self._sv._quarter_health = math.floor(self._sv._max_health / 4)
   self._sv._threequarts_health = self._sv._quarter_health * 3
   self._sv._half_health = self._sv._quarter_health * 2
end

function HealthObserver:_on_health_changed(e)
   local curr_health = self._attributes_component:get_attribute('health')
   local toast_uri = nil 
   local gaining_health = curr_health > self._sv.last_health

   --show a temporary toast if we cross a threshold
   --pick the toast based on the current health and its comparison to the last health
   if gaining_health then
      --We're gaining health
      if self._sv.last_health < self._sv._half_health and curr_health >= self._sv._half_health then
         toast_uri =  '/stonehearth/data/effects/thoughts/health/1_to_2.json' 
      elseif self._sv.last_health < self._sv._threequarts_health and curr_health >= self._sv._threequarts_health then
         toast_uri = '/stonehearth/data/effects/thoughts/health/2_to_3.json' 
      elseif self._sv.last_health < self._sv._max_health and curr_health == self._sv._max_health then
         toast_uri = '/stonehearth/data/effects/thoughts/health/3_to_4.json' 
      end
   else
      --We're losing health
      if self._sv.last_health > self._sv._threequarts_health and curr_health <= self._sv._threequarts_health then
         toast_uri = '/stonehearth/data/effects/thoughts/health/4_to_3.json' 
      elseif self._sv.last_health > self._sv._half_health and curr_health <= self._sv._half_health then
         toast_uri = '/stonehearth/data/effects/thoughts/health/3_to_2.json' 
      elseif self._sv.last_health > self._sv._quarter_health and curr_health <= self._sv._quarter_health then
         toast_uri = '/stonehearth/data/effects/thoughts/health/2_to_1.json'
      end
   end

   if toast_uri then
      self:_change_toast(toast_uri, gaining_health)
   end

   self._sv.last_health = curr_health
end

function HealthObserver:_change_toast(toast_uri, gaining_health)
   local direction_string, friendly_string

   --Is there an existing toast? If so, unthink it
   if self._sv.last_toast_uri then
      radiant.entities.unthink(self._entity, self._sv.last_toast_uri)
   end

   --Put up the next toast uri
   if toast_uri then
      radiant.entities.think(self._entity, toast_uri, stonehearth.constants.think_priorities.HEALTH)
      self._sv.last_toast_uri = toast_uri

      if gaining_health then
         direction_string = 'gain'
      else
         direction_string = 'loss'
      end

      if self:_is_friendly_to_player(self._entity) then
         friendly_string = 'friendly'
      else
         friendly_string = 'hostile'
      end

      local effect_path = string.format('/stonehearth/data/effects/thoughts/health/health_bar_%s_%s.json',
                                        direction_string, friendly_string)

      radiant.effects.run_effect(self._entity, effect_path)
   end
end

function HealthObserver:_is_friendly_to_player(entity)
   -- hard code this until we have a better solution
   return radiant.entities.get_faction(entity) == 'player_1'
end

return HealthObserver
