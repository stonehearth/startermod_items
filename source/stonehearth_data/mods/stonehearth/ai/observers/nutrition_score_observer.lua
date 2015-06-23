--[[
   Based on a series of conditions, raise or lower the food score. 
   Things that raise/lower score:
   -- not eating all day [complete]
   -- eating at least once a day (to a score cap of 50%) [complete]
   -- eating the same food X times in a row [complete]
   -- eating a different food than the last food you ate [provisionally complete, could use more phrases]
   -- eating really nutritious food [provisionally complete, could use more phrases]
   -- losing health due to malnourishment [complete]
]]

local NutritionScoreObserver = class()

--Called once on creation
function NutritionScoreObserver:initialize(entity)
   self._sv.entity = entity
   self._sv.eaten_today = false
   self._sv.malnourished_today = false
   self._sv._last_foods = {}
   self._sv.midnight_listener = stonehearth.calendar:set_alarm('0:00', function()
            self:_on_midnight()
         end)
end

--Always called. If restore, called after restore.
function NutritionScoreObserver:activate()
   self._entity = self._sv.entity
   self._score_component = self._entity:add_component('stonehearth:score')
   self._eat_listener = radiant.events.listen(self._entity, 'stonehearth:eat_food', self, self._on_eat)
   self._malnoursh_listener = radiant.events.listen(self._entity, 'stonehearth:malnourishment_event', self, self._on_malnourishment)
end

--Called when restoring
function NutritionScoreObserver:restore()
   if self._sv.midnight_listener then
      self._sv.midnight_listener:bind(function()
            self:_on_midnight()
         end)
   end
end

--Stop listening to all this stuff
function NutritionScoreObserver:destroy()
   self._eat_listener:destroy()
   self._eat_listener = nil

   self._sv.midnight_listener:destroy()
   self._sv.midnight_listener = nil

   self._malnoursh_listener:destroy()
   self._malnoursh_listener = nil
end

--Once per day, if you lose health due to malnourishment, take a hit to the nutrition score
function NutritionScoreObserver:_on_malnourishment(e)
   if not self._sv.malnourished_today then
      local journal_data = {entity = self._entity, description = 'starving', tags = {'malnourishment', 'hunger', 'food'}}
      self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.MALNOURISHMENT_PENALTY, journal_data)
      self._sv.malnourished_today = true
   end
end

--At midnight, reset stuff
function NutritionScoreObserver:_on_midnight(e)
   self:_test_not_eaten_today()

   --Reset daily counters
   self._sv.eaten_today = false
   self._sv.malnourished_today = false

   self.__saved_variables:mark_changed()
end

--Test if we've eaten anything today. If not, change score
function NutritionScoreObserver:_test_not_eaten_today()
   if not self._sv.eaten_today then
      local journal_data = {entity = self._entity, description = 'foodless', tags = {'foodless', 'hunger', 'food'}}
      self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.NO_FOOD_TODAY, journal_data)
   end
end

--When we've eaten, increase the score if we're currently under a certain amount
--Or increase/decrease based on a variety of other states
function NutritionScoreObserver:_on_eat(e)
   local food_data =  radiant.resources.load_json(e.food_uri)
   local food_name = food_data.components.unit_info.name
   local food_satisfaction = food_data.entity_data['stonehearth:food'].default.satisfaction
   self:_add_food_to_record(food_name)
   
   --If the nutrition score is < 5 you can increase it just by eating food once a day
   if self._score_component:get_score('nutrition') < stonehearth.constants.score.nutrition.SIMPLE_EATING_SCORE_CAP then
      if not self._sv.eaten_today then
         local description = 'eat_once_daily'
         if food_name == 'Plate of berries' then
            description = 'eating_berries'
         end
         local journal_data = {entity = self._entity, description = description,
            substitutions = {recent_food = food_name},
            tags = {'foodless', 'hunger', 'food'}}
         self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_ONCE_TODAY, journal_data)
      end
   else
      --if your nutrition score was >= 50, then you will lose a point if you've only eaten 1 kind of food recently
      local food_name = food_data.components.unit_info.name
      if self:_foods_are_homogenous(food_name) then
         local journal_data = {entity = self._entity, description = 'flavor_fatigue', substitutions = {recent_food = food_name}, tags = {'repetition', 'food'}}
         self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_SAME_FOODS, journal_data)
      end
   end
   
   --You can also gain a point if the food you just ate is different from the last food you just ate
   if self._sv._last_foods[2] and self._sv._last_foods[1] ~= self._sv._last_foods[2] then
      local journal_data = {entity = self._entity, description = 'different_food', substitutions = {recent_food = food_name}, tags = {'repetition', 'food'}}
      self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_DIFFERENT_FOODS, journal_data)
   end
 
   --You can also gain a point if its really, really nutritious
   if food_satisfaction >= stonehearth.constants.score.nutrition.NUTRITION_THRESHHOLD then
      local journal_data = {entity = self._entity, description = 'super_nutritious_food', substitutions = {recent_food = food_name}, tags = {'repetition', 'food'}}
      self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_NUTRITIOUS_FOOD, journal_data)
   end

   --When you eat, also look at all the count of the types of foods available
   self:_update_food_type_score()

   --Originally, had a point here about eating at mealtime, but maybe move that to "belonging score"

   --Mark it true that we've eaten today
   self._sv.eaten_today = true
   self.__saved_variables:mark_changed()
end

--Add the food to the front/top of our record. Trim the record if we need to. 
function NutritionScoreObserver:_add_food_to_record(new_food_name)
   table.insert(self._sv._last_foods, 1, new_food_name)
   if #self._sv._last_foods > stonehearth.constants.score.nutrition.HOMOGENEITY_THRESHHOLD then
      --remove the new last element
      table.remove(self._sv._last_foods, #self._sv._last_foods)
   end
end

--Checks to see if the entity has eaten the same thing x times in a row
--Where x is determined by the homogeneity threshhold (now 5)
function NutritionScoreObserver:_foods_are_homogenous(new_food_name)
   if #self._sv._last_foods < stonehearth.constants.score.nutrition.HOMOGENEITY_THRESHHOLD then
      --not enough foods to be sick of the food yet
      return false
   end

   --There are enough things in the array to have a decent history
   --iterate through the contents. If everything in there is the same, return true. Otherwise, return false.
   for i, v in ipairs(self._sv._last_foods) do 
      if v ~= new_food_name then
         --Woo, I ate something different from the foods I've eaten recently, return false
         return false
      end
   end

   --If we got here, all the foods were the same as the new food put in. so bored of food now; return true. 
   return true
end

function NutritionScoreObserver:_update_food_type_score()
   local raw_count = stonehearth.food_decay:get_raw_food_count()
   local prepared_count = stonehearth.food_decay:get_prepared_food_count()
   local luxury_count = stonehearth.food_decay:get_luxury_food_count()

   local raw_food_score = stonehearth.constants.score.food.RAW_FOOD_COUNT_MULTIPLIER * raw_count
   local prepared_food_score = stonehearth.constants.score.food.PREPARED_FOOD_COUNT_MULTIPLIER * prepared_count
   local luxury_food_score = stonehearth.constants.score.food.LUXURY_FOOD_COUNT_MULTIPLIER * luxury_count

   if prepared_count <= 0 then
      prepared_food_score = stonehearth.constants.score.food.NO_PREPARED_FOOD_PENALTY
   end
   
   if luxury_count <= 0 then
      luxury_food_score = stonehearth.constants.score.food.NO_LUXURY_FOOD_PENALTY
   end

   self._score_component:change_score('raw_food', raw_food_score)
   self._score_component:change_score('prepared_food', prepared_food_score)
   self._score_component:change_score('luxury_food', luxury_food_score)
end

return NutritionScoreObserver