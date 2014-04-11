--[[
Counts down from 100 (well nourished) to 25 (starving) and 0 (min calories)
See the documentation for food/calorie/eating behavior here: http://wiki.rad-ent.com/doku.php?id=hunger
]]

local calendar = stonehearth.calendar

local CalorieObserver = class()

--TODO: now defunct?
function CalorieObserver:__init(entity)
end

function CalorieObserver:initialize(entity, json)
   self._entity = entity
   self._eat_task = nil
   self._attributes_component = entity:add_component('stonehearth:attributes')

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.should_be_eating = false
   end

   --Hunger is on by default. If it's NOT on, don't do any of this. 
   self._enable_hunger = radiant.util.get_config('enable_hunger', true)
   if self._enable_hunger then
      radiant.events.listen(calendar, 'stonehearth:hourly', self, self._on_hourly)
      radiant.events.listen(self._entity, 'stonehearth:attribute_changed:calories', self, self._on_calories_changed)
   
      --Also, should we be eating right now? If so, let's do that
      if self._sv.should_be_eating then
         self:_start_eat_task()
      end
   end
end

function CalorieObserver:destroy()
   if self._enable_hunger then
      radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self._on_hourly)
      radiant.events.unlisten(self._entity, 'stonehearth:attribute_changed:calories', self, self._on_calories_changed)
   end
end

--- If calories change for any reason, adjust health
function CalorieObserver:_on_calories_changed()
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

      --Set the journal entry
      radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                             {entity = self._entity, description = 'starving'})

      if not self._is_malnourished then
         self._is_malnourished = true
         radiant.entities.add_buff(self._entity, 'stonehearth:buffs:starving')

         self:_start_eat_task()
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
      end

      --If our calorie count is >= max, then stop the eating task
      if calories >= stonehearth.constants.food.MAX_ENERGY and self._eat_task then
         self:_finish_eating()
      end
   end
end

--- Every hour, handle energy, eating impulses, health etc
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

   --If it's mealtime, everyone go eat. 
   self:_handle_mealtimes(e.now.hour)
end

-- If it's mealtime, start the looking for food task
-- TODO: is this necessary: If it's not mealtime, and we're not malnourished, stop eating
function CalorieObserver:_handle_mealtimes(hour)
   if hour == stonehearth.constants.food.MEALTIME_START then
      self:_start_eat_task()
   end
end

--If the task doesn't currently exist, start the task to look for food
function CalorieObserver:_start_eat_task()
   --show the hungry toast, if not yet showing from mealtime
   radiant.entities.think(self._entity, '/stonehearth/data/effects/thoughts/hungry', stonehearth.constants.think_priorities.HUNGRY)

   if not self._eat_task then
      -- ask the ai component for the task group for `basic needs` and create
      -- a task to eat at the proper priority.
      self._eat_task = self._entity:get_component('stonehearth:ai')
                                       :get_task_group('stonehearth:basic_needs')
                                          :create_task('stonehearth:eat', {})
                                             :set_priority(stonehearth.constants.priorities.basic_needs.EAT)
                                             :start()

      self._sv.should_be_eating = true
      self.__saved_variables:mark_changed()
   end
end

function CalorieObserver:_finish_eating()
   --Hide the hungry thought toast, if it was in effect
   radiant.entities.unthink(self._entity, '/stonehearth/data/effects/thoughts/hungry')
   
   self._sv.should_be_eating = false
   self.__saved_variables:mark_changed()

   self._eat_task:destroy()
   self._eat_task = nil
end

return CalorieObserver