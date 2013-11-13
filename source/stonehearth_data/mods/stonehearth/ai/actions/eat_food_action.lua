--[[
   Run to a food dish and serve self from it. 
--]]
radiant.mods.load('stonehearth')
local priorities = require('constants').priorities.needs
local event_service = require 'services.event.event_service'
local Point3 = _radiant.csg.Point3

local EatFoodAction = class()

EatFoodAction.name = 'eat food'
EatFoodAction.does = 'stonehearth:eat_food'
EatFoodAction.priority = 5  --The minute this is called, it runs. 

--Some constants
local CLOSE_DISTANCE = 3
local FAR_DISTANCE = 6
local STANDING_MODIFIER = 0.8
local CHAIR_MODIFIER = 1.2

function EatFoodAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity         --the game character
   self._ai = ai

   self._looking_for_seat = false
   self._seat = nil
   self._path_to_seat = nil
   self._pathfinder = nil
   self._hunger = nil

   radiant.events.listen(entity, 'stonehearth:attribute_changed:hunger', self, self.on_hunger_changed)
 end

function EatFoodAction:on_hunger_changed(e)
   self._hunger = e.value
   if self._hunger >= 80  then
      self:start_looking_for_seat()
   else
      -- we're not hungry. If we were looking for a place to eat, stop
      self:stop_looking_for_seat()
   end
end

function EatFoodAction:start_looking_for_seat()
   if self._looking_for_seat then
      return
   end
   assert(not self._pathfinder)
   self._looking_for_seat = true;
   if not self._pathfinder then
      self:find_seat()
   end
end

function EatFoodAction:stop_looking_for_seat()
   -- tell the pathfinder to stop searching in the background
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   self._looking_for_seat = false
end

function EatFoodAction:find_seat()
   assert(not self._pathfinder)

   local filter_fn = function(item)
      local is_bed = radiant.entities.get_entity_data(item, 'stonehearth:chair')
      local lease = item:get_component('stonehearth:lease_component')
      if is_bed ~= nil and lease ~= nil then
         return lease:can_acquire_lease(self._entity)
      else
         return false
      end
   end

   -- save the path to the chair once we find it
   local solved_cb = function(path)
      self._seat = path:get_destination()
      self._path_to_seat = path
      if self._hunger > 80 then
         self._ai:set_action_priority(self, priorities.HUNGRY)
      end
   end

   -- go find the path to the food
   radiant.log.info('%s creating pathfinder to find a seat', tostring(self._entity));
   local desc = string.format('finding a seat for %s', tostring(self._entity))
   self._pathfinder = radiant.pathfinder.create_path_finder(desc)
                        :set_source(self._entity)
                        :set_filter_fn(filter_fn)
                        :set_solved_cb(solved_cb)
                        :find_closest_dst()

   assert(self._pathfinder)
end

function EatFoodAction:run(ai, entity, food)
   local attributes_component = entity:add_component('stonehearth:attributes')
   local hunger = attributes_component:get_attribute('hunger')
   local entity_loc = entity:get_component('mob'):get_world_grid_location()
   local satisfaction = food:get_component('stonehearth:attributes'):get_attribute('sates_hunger')
   local consumption_text = ''
   local worker_name = radiant.entities.get_display_name(entity)

   if hunger >= 120 then
      --We're starving! It doesn't matter if we have a seat.
      --go a few steps away and run the eating animation while standing
      local x = math.random(0 - CLOSE_DISTANCE, CLOSE_DISTANCE)
      local z = math.random(0 - CLOSE_DISTANCE, CLOSE_DISTANCE)
      ai:execute('stonehearth:goto_location', Point3(entity_loc.x + x, entity_loc.y, entity_loc.z + z))
      --TODO: replace with standing/eating animation
      ai:execute('stonehearth:run_effect', 'work')
      satisfaction = satisfaction * STANDING_MODIFIER
      consumption_text = radiant.entities.get_entity_data(food, 'stonehearth:message_starved')

   elseif hunger >= 80 then
      --if we have a path, grab the seat's lease, goto the seat, sit, and eat
      if self._path_to_seat and self._seat:add_component('stonehearth:lease_component'):try_to_acquire_lease(entity) then 
         --We successfully acquired the lease
         radiant.log.info('leasing %s to %s', tostring(self._seat), tostring(self._entity))
         ai:execute('stonehearth:goto_entity', self._seat)

         self:_register_for_chair_events()

         --play sitting and eating animation
         ai:execute('stonehearth:run_effect', 'work')
         --TODO: alter the chair modifier depending on distance from table, kind of chair
         satisfaction = satisfaction * CHAIR_MODIFIER
         consumption_text = radiant.entities.get_entity_data(food, 'stonehearth:message_normal')
      else
         --We don't have a path or didn't get the lease. Look for any place nearby
         local x = math.random(0 - FAR_DISTANCE, FAR_DISTANCE)
         local z = math.random(0 - FAR_DISTANCE, FAR_DISTANCE)
         ai:execute('stonehearth:goto_location', Point3(entity_loc.x + x, entity_loc.y, entity_loc.z + z))
         --TODO: replace with sitting/eating animation
         ai:execute('stonehearth:run_effect', 'work')
         consumption_text = radiant.entities.get_entity_data(food, 'stonehearth:message_floor')
      end
   end

   --Decrement hunger by amount relevant to the food type
   local entity_attribute_comp = entity:get_component('stonehearth:attributes')
   local hunger = entity_attribute_comp:get_attribute('hunger')
   entity_attribute_comp:set_attribute('hunger', hunger - satisfaction)

   -- Log successful consumption
   event_service:add_entry(worker_name .. ': ' .. consumption_text .. '(+' .. satisfaction .. ')')

   --Drop carrying and destroy
   radiant.entities.drop_carrying_on_ground(entity, entity:get_component('mob'):get_world_grid_location())
   radiant.entities.destroy_entity(food)
end

function EatFoodAction:_register_for_chair_events()
   assert(self._seat, 'register_for_chair_events called without chair')
   -- If the chair moves or is destroyed while we're eating, abort
   self._chair_moved_promise = radiant.entities.on_entity_moved(self._seat, function()
      ai:abort()
   end);
   radiant.entities.on_destroy(self._seat, function()
      --Can't call ai:abort, may not be on the correct thread.
      self._seat = nil
   end);
end

function EatFoodAction:_release_seat_reservation()
   if self._seat then
      local seat_lease = self._seat:get_component('stonehearth:lease_component')
      seat_lease:release_lease(self._entity)
      self._seat = nil
      self._path_to_seat = nil
   end
end

-- Note: We don't stop the pf etc here because the only
-- reason to stop looking for somewhere to eat is
-- because we're no longer hungry
function EatFoodAction:stop()
   if self._chair_moved_promise then
      self._chair_moved_promise:destroy()
      self._chair_moved_promise = nil
   end

   --TODO: figure out sitting down posture
   --radiant.entities.stand_up(self._entity)
   self:_release_seat_reservation()


end

return EatFoodAction
