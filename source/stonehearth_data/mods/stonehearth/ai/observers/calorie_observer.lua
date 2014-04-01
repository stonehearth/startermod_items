--[[
Counts down from 100 (well nourished) to 25 (starving) and 0 (min calories)
See the documentation for food/calorie/eating behavior here: http://wiki.rad-ent.com/doku.php?id=hunger
]]

local calendar = stonehearth.calendar

local CalorieObserver = class()

--TODO: Is this init still valid? 
function CalorieObserver:__init(entity)
   self._entity = entity
   self._eat_task = nil

   self._attributes_component = entity:add_component('stonehearth:attributes')

   --Hunger is on by default. If it's NOT on, don't do any of this. 
   self._enable_hunger = radiant.util.get_config('enable_hunger', true)
   if self._enable_hunger then
      radiant.events.listen(calendar, 'stonehearth:hourly', self, self._on_hourly)
      radiant.events.listen(self._entity, 'stonehearth:attribute_changed:calories', self, self._calories_changed)
   end
end

function CalorieObserver:destroy()
   if self._enable_hunger then
      radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self._on_hourly)
      radiant.events.unlisten(self._entity, 'stonehearth:attribute_changed:calories', self, self._calories_changed)
   end
end

--- If calories change for any reason, adjust health
function CalorieObserver:_calories_changed()
   self:_adjust_health_and_status()
end

--- Change health and set status based on calorie level
--  If we are above starvation, remove the buff
--  Whenever we're malnourished, change the HP.
--  Note: only takes effect if enable_hunger user setting is turned on
--  TODO: add the toasts to the AI during mealtimes in addition to during starving
function CalorieObserver:_adjust_health_and_status()
   local calories = self._attributes_component:get_attribute('calories')
   local hp = self._attributes_component:get_attribute('health')
   local max_hp = self._attributes_component:get_attribute('max_health')
   local malnourished = calories <= stonehearth.constants.food.MALNOURISHED

   if malnourished then
      --Reduce HP (until it hits zero)
      hp = hp - stonehearth.constants.food.HOURLY_HP_LOSS
      if hp < 0 then 
         hp = 0
      end
      self._attributes_component:set_attribute('health', hp )
      if not self._is_malnourished then

         --show the hungry toast, if not yet showing from mealtime
         radiant.entities.think(self._entity, '/stonehearth/data/effects/thoughts/hungry', stonehearth.constants.think_priorities.HUNGRY)

         self._is_malnourished = true
         radiant.entities.add_buff(self._entity, 'stonehearth:buffs:starving')

         --If the task doesn't currently exist, start the task to look for food
         if not self._eat_task then
            local player_id = radiant.entities.get_player_id(self._entity)
            local town = stonehearth.town:get_town(player_id)
            self._eat_task = town:command_unit_scheduled(self._entity, 'stonehearth:eat')
               :start()
         end
      end
   elseif not malnourished then
      --increase HP (until it hits max)
      hp = hp + stonehearth.constants.food.HOURLY_HP_GAIN
      if hp > max_hp then
         hp = max_hp
      end
      self._attributes_component:set_attribute('health', hp )
      
      if self._is_malnourished then
         self._is_malnourished = false
         --TODO: discover why despair isn't being removed
         radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:starving')

         --Hide the hungry thought toast, if it was in effect
         radiant.entities.unthink(self._entity, '/stonehearth/data/effects/thoughts/hungry')
      end

      --If our calorie count is >= max, then stop the eating task
      if calories >= stonehearth.constants.food.MAX_ENERGY and self._eat_task then
         self._eat_task:destroy()
         self._eat_task = nil
      end
   end
end

--- Every hour, lose a certain amount of energy
function CalorieObserver:_on_hourly(e)
   local calories = self._attributes_component:get_attribute('calories')
   local previous_calories = calories
   calories = calories - stonehearth.constants.food.HOURLY_ENERGY_LOSS
   if calories < stonehearth.constants.food.MIN_ENERGY then
      calories = stonehearth.constants.food.MIN_ENERGY
   end

   -- We should adjust health/status every hour even if we bottomed/maxed out on calories
   if calories == previous_calories then
      self:_adjust_health_and_status()
   end
   self._attributes_component:set_attribute('calories', calories)
end

return CalorieObserver