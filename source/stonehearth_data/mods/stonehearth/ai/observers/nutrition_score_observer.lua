--[[
   Based on a series of conditions, raise or lower the food score. 
   Things that raise/lower score:
   -- not eating all day
   -- eating at least once a day (to a score cap of 50%)
   -- eating the same food X times in a row
   -- eating a different food than the last food you ate
   -- eating really nutritious food
   -- losing health due to malnourishment [complete]
]]

local NutritionScoreObserver = class()

local calendar = stonehearth.calendar

function NutritionScoreObserver:__init(entity)
end

function NutritionScoreObserver:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   self._score_component = entity:add_component('stonehearth:score')


   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.eaten_today = false
      self._sv.malnourished_today = false
      self._sv._last_foods = {}
   end

   radiant.events.listen(entity, 'stonehearth:eat_food', self, self._on_eat)
   radiant.events.listen(calendar, 'stonehearth:midnight', self, self._on_midnight)
   radiant.events.listen(entity, 'stonehearth:malnourishment_event', self, self._on_malnourishment)
end

--Stop listening to all this stuff
function NutritionScoreObserver:destroy()
   radiant.events.unlisten(self._entity, 'stonehearth:eat_food', self, self._on_eat)
   radiant.events.unlisten(calendar, 'stonehearth:midnight', self, self._on_midnight)
   radiant.events.unlisten(self._entity, 'stonehearth:malnourishment_event', self, self._on_malnourishment)
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
         self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_ONCE_TODAY)
      end
   else
      --if your nutrition score was >= 50, then you will lose a point if you've only eaten 1 kind of food recently
      local food_name = food_data.components.unit_info.name
      if self:_foods_are_homogenous(food_name) then
         self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_SAME_FOODS)
      end
   end
   
   --You can also gain a point if the food you just ate is different from the last food you just ate
   if self._sv._last_foods[2] and self._sv._last_foods[1] ~= self._sv._last_foods[2] then
      self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_DIFFERENT_FOODS)
   end
 
   --You can also gain a point if its really, really nutritious
   if food_satisfaction >= stonehearth.constants.score.nutrition.NUTRITION_THRESHHOLD then
      self._score_component:change_score('nutrition', stonehearth.constants.score.nutrition.EAT_NUTRITIOUS_FOOD)
   end

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

return NutritionScoreObserver