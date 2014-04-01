--[[
Counts down from 100 (well nourished) to 25 (starving) and 0 (min calories)
See the documentation for food/calorie/eating behavior here: http://wiki.rad-ent.com/doku.php?id=hunger
]]

local calendar = stonehearth.calendar

local CalorieObserver = class()

--TODO: Is this init still valid? 
function CalorieObserver:__init(entity)
   self._entity = entity
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

--- If calories fall under a certain level, add starving debuff
--  If we are above starvation, remove the buff
--  Whenever we're malnourished, change the HP.
--  TODO: if we fall under 0, initiate death by starvation (lower priority than other kinds of death)
--  TODO: only do this if hunger is turned on (as a user setting)
--  TODO: add the toasts to the AI during mealtimes
function CalorieObserver:_calories_changed()
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
         radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:starving')

         --Hide the hungry thought toast, if it was in effect
         radiant.entities.unthink(self._entity, '/stonehearth/data/effects/thoughts/hungry')
      end
   end
end

--- Every hour, lose a certain amount of energy
function CalorieObserver:_on_hourly(e)
   local calories = self._attributes_component:get_attribute('calories')
   calories = calories - stonehearth.constants.food.HOURLY_ENERGY_LOSS
   if calories < 0 then
      calories = 0
   end
   self._attributes_component:set_attribute('calories', calories)
end

return CalorieObserver