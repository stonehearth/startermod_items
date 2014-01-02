--[[
   Listen on changes to hunger attribute. If > 80, start looking for optimal food.
   If > 120, set starving debuff. Once we've found food, set priority of eating it
   based on how hungry we are. 
--]]
radiant.mods.load('stonehearth')
local priorities = require('constants').priorities.needs

local FindFoodAction = class()
local log = radiant.log.create_logger('actions.find_food')

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

   radiant.events.listen(radiant.events, 'stonehearth:very_slow_poll', self, self.on_poll)
end

function FindFoodAction:on_poll()
   self._hunger = radiant.entities.get_attribute(self._entity, 'hunger')
   self:check_hunger()
end

--- Changes behavior based on hunger
--  The first time hunger > 120, add starving debuff and set priority high 
--  so we run the starving animation regardless of whether we've found food.
--  When > 80, look for food. If < 80, we're not hungry, so stop
--  looking for food. 
--  @param e - e.value is the status of hunger
function FindFoodAction:check_hunger()   
   if self._hunger >= 120 then
      radiant.entities.add_buff(self._entity, 'stonehearth:buffs:starving')
   end

   if self._hunger >= 80  then
      radiant.entities.think(self._entity, '/stonehearth/data/effects/thoughts/hungry')
      self:start_looking_for_food()
   else   
      if self._looking_for_food then
         self:stop_looking_for_food()
      end

      radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:starving')
      radiant.entities.unthink(self._entity, '/stonehearth/data/effects/thoughts/hungry')
      
      self._ai:set_action_priority(self, 0)
   end
end

--- Kick off pathfinder to look for good food 
function FindFoodAction:start_looking_for_food()
   if self._looking_for_food then
      return
   end
   assert(not self._pathfinder)
   self._looking_for_food = true;
   if not self._pathfinder then
      self:find_good_food()
   end
end

--- Tell pf we don't need to look for food anymore. 
function FindFoodAction:stop_looking_for_food()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   self._looking_for_food = false
end

--- Make a pf to find good food to eat.
--  Right now this means that it's fruit AND food. 
--  TODO: Make "preferred foods" different based on person and food type. Have
--  people go for unpreferred foods if they're hungry enough. 
--  Once we've found food, set priority based on hunger, so if we're
--  really hungry, we can go eat right away. 
function FindFoodAction:find_good_food()
   assert(not self._pathfinder)

   local filter_fn = function(item)
      local material = item:get_component('stonehearth:material')
      local is_good_food = material and material:is('food')
      return is_good_food
   end

   local solved_cb = function(path)
      self._food = path:get_destination()
      self._path_to_food = path

      if self._hunger >= 120 then
         self._ai:set_action_priority(self, priorities.REALLY_HUNGRY)
      elseif self._hunger >= 100 then
         self._ai:set_action_priority(self, priorities.HUNGRY)
      elseif self._hunger >=80 then
         self._ai:set_action_priority(self, priorities.BARELY_HUNGRY)
      end
   end

   -- go find the path to the food
   log:debug('%s creating pathfinder to find food', tostring(self._entity));
   local desc = string.format('finding food for %s', tostring(self._entity))
   self._pathfinder = radiant.pathfinder.create_path_finder(self._entity, desc)
                        :set_filter_fn(filter_fn)
                        :set_solved_cb(solved_cb)
                        :find_closest_dst()

   assert(self._pathfinder)
end

--- If we have a path to food, go get it. If not
--  and if hunger > 120, run a despairing animation
function FindFoodAction:run(ai, entity)
   assert(self._path_to_food)
   self:stop_looking_for_food()
   ai:execute('stonehearth:get_food', self._path_to_food)
end

--- Whether we're eating or have run the despair animation or just
--  got interrupted, set priority to 0. It will go up again as soon
--  as we've found some food. 
function FindFoodAction:stop()
   self:stop_looking_for_food()
   self._ai:set_action_priority(self, 0)
end

return FindFoodAction
