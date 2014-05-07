--[[
   Things that change your energy score:
   - starting the game: defaults to 5
   - missing mealtime: -1
   - eating at mealtime: +2
   - eating at least once per day, not at mealtime: +1
   - eating quality food: +1
   - eating something different than what I ate last time: +1
   - eating 3 of the same things in a row -1
   - being malnourished: -3
]]

local EnergyScoreObserver = class()

local calendar = stonehearth.calendar

function EnergyScoreObserver:__init(entity)
end

function EnergyScoreObserver:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   self._score_component = entity:add_component('stonehearth:score')


   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.eaten_today = false
   end

   --listen on midnight to reset daily counters
   radiant.events.listen(calendar, 'stonehearth:midnight', self, self._on_midnight)
end

function EnergyScoreObserver:destroy()
   radiant.events.unlisten(calendar, 'stonehearth:midnight', self, self._on_midnight)
end

function EnergyScoreObserver:_on_midnight(e)
   self:_test_not_eaten_today()
end

--Test if we've eaten anything today. If not, change score
function EnergyScoreObserver:_test_not_eaten_today()
   if not self._sv.eaten_today then
      self._score_component:change_score('energy', stonehearth.constants.score.energy.NO_FOOD_TODAY)
   end
   self._sv.eaten_today = false
end

return EnergyScoreObserver