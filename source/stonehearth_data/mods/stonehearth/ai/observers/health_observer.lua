--[[
   Observes the HP of the entity and shows popup toasts based on their current HP
]]

local HealthObserver = class()

function HealthObserver:initialize(entity, json)
   self._entity = entity
   self._attributes_component = entity:add_component('stonehearth:attributes')
   self._timer_duration = '2m'

   --After everything has been initialized, then set the current 
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true

      --When everything has been init for the first time, store the max health
      --Listen on health changes only after we have a base health
      --radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         self._sv.health = self._attributes_component:get_attribute('max_health')
         self:_calculate_thresholds()
         radiant.events.listen(self._entity, 'stonehearth:attribute_changed:health', self, self._on_health_changed)
         --return radiant.events.UNLISTEN
      --end)
   else
      --TODO: test this
      if self._sv._expiration_time then
         radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
            local duration = self._sv._expiration_time - stonehearth.calendar:get_elapsed_time()
            self:_change_toast(self._sv.curr_toast_uri, duration)
            return radiant.events.UNLISTEN
         end)      
      end
      
      radiant.events.listen(self._entity, 'stonehearth:attribute_changed:health', self, self._on_health_changed)
   end
end

-- We care about full health, three quarters, half, and quarter health
-- Given full health, calculate the others
function HealthObserver:_calculate_thresholds()
   self._sv._max_health = self._sv.health
   self._sv._quarter_health = math.floor(self._sv._max_health / 4)
   self._sv._threequarts_health = self._sv._quarter_health * 3
   self._sv._half_health = self._sv._quarter_health * 2
end

function HealthObserver:_on_health_changed(e)
   local curr_health = self._attributes_component:get_attribute('health')
   local next_health_toast = nil 

   --show a temporary toast if we cross a threshold
   --pick the toast based on the current health and its comparison to the last health
   if curr_health > self._sv.health then
      --We're gaining health
      if self._sv.health < self._sv._half_health and curr_health >= self._sv._half_health then
         next_health_toast =  '/stonehearth/data/effects/thoughts/health/half_gain.json' 
      elseif self._sv.health < self._sv._threequarts_health and curr_health >= self._sv._threequarts_health then
         next_health_toast = '/stonehearth/data/effects/thoughts/health/threequarts_gain.json' 
      elseif self._sv.health < self._sv._max_health and curr_health == self._sv._max_health then
         next_health_toast = '/stonehearth/data/effects/thoughts/health/full_gain.json' 
      end
   else
      --We're losing health
      if self._sv.health > self._sv._threequarts_health and curr_health <= self._sv._threequarts_health then
         next_health_toast = '/stonehearth/data/effects/thoughts/health/threequarts_loss.json' 
      elseif self._sv.health > self._sv._half_health and curr_health <= self._sv._half_health then
         next_health_toast = '/stonehearth/data/effects/thoughts/health/half_loss.json' 
      elseif self._sv.health > self._sv._quarter_health and curr_health <= self._sv._quarter_health then
         next_health_toast = '/stonehearth/data/effects/thoughts/health/quarter_loss.json'
      end
   end

   if next_health_toast then
      self:_change_toast(next_health_toast, self._timer_duration)
      self._sv.curr_toast_uri = next_health_toast
   end

   self._sv.health = curr_health
end

function HealthObserver:_change_toast(next_toast_uri, time)
   --Is there an existing toast? If so, unthink it
   if self._sv.curr_toast_uri then
      radiant.entities.unthink(self._entity, self._sv.curr_toast_uri)
   end
   --Is there an existing timer? If so, stop it
   if self._timer then
      self:_stop_timer()
   end

   --Put up the next toast uri
   if next_toast_uri then
      radiant.entities.think(self._entity, next_toast_uri, stonehearth.constants.think_priorities.HEALTH)
   
      --set the expiration timer
      self._timer = stonehearth.calendar:set_timer(time, function() 
         radiant.entities.unthink(self._entity, next_toast_uri)
      end)
      self._sv._expiration_time = self._timer:get_expire_time()
   end
end

function HealthObserver:_stop_timer()
   self._timer:destroy()
   self._timer = nil
   self._sv.timer_expiration = nil
end

return HealthObserver