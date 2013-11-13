--[[
   Listen on changes to hunger attribute. If > 80, start looking for optimal food.
   If >100, set priority to 5. If > 120, set priority to 10. 
   Sleep always wins over food.
--]]
radiant.mods.load('stonehearth')
local priorities = require('constants').priorities.needs


local FindFoodAction = class()

FindFoodAction.name = 'find food'
FindFoodAction.does = 'stonehearth:top'
FindFoodAction.priority = 0

function FindFoodAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity         --the game character
   self._ai = ai

   self._hunger = nil

   self._pathfinder = nil
   self._food = nil
   self._path_to_food = nil

   self._looking_for_food = false

   radiant.events.listen(entity, 'stonehearth:attribute_changed:hunger', self, self.on_hunger_changed)
end

function FindFoodAction:on_hunger_changed(e)
   self._hunger = e.value
   
   --TODO: debuffs!

   if self._hunger >= 120 then
      --We're starving! Set priority higher!
      --TODO: if old pathfinder has not yet returned, dump it and 
      --start a new one that will look for any food.
      --(Extra bonus in cases where we missed the 80-100 window)
      self._ai:set_action_priority(self, priorities.REALLY_HUNGRY)
      --TODO: add "starved" debuff
   elseif self._hunger >= 100  then
      --If we haven't eaten yet, our priority was too low. Set it to 10
      self._ai:set_action_priority(self, priorities.HUNGRY)
   elseif self._hunger >= 80  then
      --self._ai:set_action_priority(self, priorities.BARELY_HUNGRY)
      self:start_looking_for_best_food()
   else
      -- we're not hungry. If we were looking for food, stop
      if self._looking_for_food then
         self:stop_looking_for_food()
      end
      self._ai:set_action_priority(self, 0)
   end
end

function FindFoodAction:start_looking_for_best_food()
   if self._looking_for_food then
      return
   end
   
   assert(not self._pathfinder)
   
   self._looking_for_food = true;

   if not self._pathfinder then
      self:find_good_food()
   end
end

function FindFoodAction:stop_looking_for_food()
   -- tell the pathfinder to stop searching in the background
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   self._looking_for_food = false
end

--[[
   Find good food to eat. 
   Right now this means that it's fruit AND food. 
   Later we may relax our criteria to just food.  
--]]
function FindFoodAction:find_good_food()
   assert(not self._pathfinder)

   local filter_fn = function(item)
      local is_good_food = item:get_component('stonehearth:material'):is('fruit food')
      return is_good_food
   end

   -- save the path to the food once we find it
   local solved_cb = function(path)
      self._food = path:get_destination()
      self._path_to_food = path
      self._ai:set_action_priority(self, priorities.BARELY_HUNGRY)
   end

   -- go find the path to the food
   radiant.log.info('%s creating pathfinder to find food', tostring(self._entity));
   local desc = string.format('finding food for %s', tostring(self._entity))
   self._pathfinder = radiant.pathfinder.create_path_finder(desc)
                        :set_source(self._entity)
                        :set_filter_fn(filter_fn)
                        :set_solved_cb(solved_cb)
                        :find_closest_dst()

   assert(self._pathfinder)
end

function FindFoodAction:run(ai, entity)
   if self._path_to_food then
      self:stop_looking_for_food()
      ai:execute('stonehearth:serve_food', self._path_to_food)
   elseif self._hunger >= 120 then
      -- Cry in despair because we're so hungry
      --self._ai:execute('stonehearth:???')
   end
end

--[[
   If the action is stopped, stop the pathfinder
--]]
function FindFoodAction:stop()
   if self._hunger < 80 then 
      self:stop_looking_for_food()
   end
end

return FindFoodAction
