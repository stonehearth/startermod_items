--[[
Counts down from 100 (well nourished) to 25 (starving) and 0 (min calories)
See the documentation for food/calorie/eating behavior here: http://wiki.rad-ent.com/doku.php?id=hunger
]]

local CalorieObserver = class()

function CalorieObserver:__init(entity)
   self._eat_task = nil
   self._enable_hunger = radiant.util.get_config('enable_hunger', true)
end

--Called once on creation
function CalorieObserver:initialize(entity)
   self._sv.entity = entity
   self._sv.should_be_eating = false
   if self._enable_hunger then
      self._sv.hour_listener = stonehearth.calendar:set_interval("CalorieObserver on_hourly", '1h', 
         radiant.bind(self, '_on_hourly'))
   end
end

--Always called. If restore, called after restore.
function CalorieObserver:activate()
   self._entity = self._sv.entity
   self._attributes_component = self._entity:add_component('stonehearth:attributes')
   if self._enable_hunger then
      self._calorie_listener = radiant.events.listen(self._entity, 'stonehearth:attribute_changed:calories', self, self._on_calories_changed)
      
      --Also, should we be eating right now? If so, let's do that
      if self._sv.should_be_eating then
         self:_start_eat_task()
      end
   end
end

function CalorieObserver:destroy()
   if self._enable_hunger then
      self._sv.hour_listener:destroy()
      self._sv.hour_listener = nil

      self._calorie_listener:destroy()
      self._calorie_listener = nil
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

      --Send general malnourishment notice
      radiant.events.trigger_async(self._entity, 'stonehearth:malnourishment_event')

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
         radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:starving')
      end

      --If our calorie count is >= max, then stop the eating task
      if calories >= stonehearth.constants.food.MAX_ENERGY and self._eat_task then
         self:_finish_eating()
      end
   end
end

--- Every hour, handle energy, eating impulses, health etc
function CalorieObserver:_on_hourly()
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
   self:_handle_mealtimes(calories)
end

-- If it's mealtime, start the looking for food task
-- TODO: is this necessary: If it's not mealtime, and we're not malnourished, stop eating
function CalorieObserver:_handle_mealtimes(calories)
   local now = stonehearth.calendar:get_time_and_date()   
   if now.hour == stonehearth.constants.food.MEALTIME_START and calories < stonehearth.constants.food.MAX_ENERGY then
      self:_start_eat_task()
   end
end

--If the task doesn't currently exist, start the task to look for food
function CalorieObserver:_start_eat_task()
   --show the hungry toast, if not yet showing from mealtime
   if not self._thought then 
      self._thought = radiant.entities.think(self._entity, '/stonehearth/data/effects/thoughts/hungry', stonehearth.constants.think_priorities.HUNGRY)
   end

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
   self._sv.should_be_eating = false
   self.__saved_variables:mark_changed()

   if self._thought then
      self._thought:destroy()
      self._thought = nil
   end
   if self._eat_task then
      self._eat_task:destroy()
      self._eat_task = nil
   end
end

return CalorieObserver