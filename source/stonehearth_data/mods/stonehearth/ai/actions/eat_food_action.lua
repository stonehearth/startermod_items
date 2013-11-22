--[[
   Action is called once an entity serves itself from a food source.
   Depending on the hunger level, the entity will either find a chair
   that his pathfinder has located earlier, or will eat on the ground
   some distance from the bush. Different eating actions yield a different
   amounts of satisfaction, meaning it might be longer before a person 
   has to eat again. 
--]]
radiant.mods.load('stonehearth')
local priorities = require('constants').priorities.needs
local event_service = require 'services.event.event_service'
local personality_service = require 'services.personality.personality_service'
local Point3 = _radiant.csg.Point3

local EatFoodAction = class()

EatFoodAction.name = 'stonehearth_eat_food'
EatFoodAction.does = 'stonehearth:eat_food'
EatFoodAction.priority = 5       --The minute this is called, it runs. 

--Some constants
local CLOSE_DISTANCE = 3         --Sit somewhere nearby to eat
local FAR_DISTANCE = 6           --Sit a little further away to eat
local HURRIED_MODIFIER = 0.80    --Eating in a hurry is less satisfying
local CHAIR_MODIFIER = 1.2       --Eating while sitting is more satisfying

function EatFoodAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity         
   self._ai = ai
   self._looking_for_seat = false
   self._seat = nil
   self._path_to_seat = nil
   self._pathfinder = nil
   self._hunger = nil
   self._food = nil

   radiant.events.listen(entity, 'stonehearth:attribute_changed:hunger', self, self.on_hunger_changed)
 end

--- Whenever the entity is hungry, subliminally start looking for a seat
function EatFoodAction:on_hunger_changed(e)
   self._hunger = e.value
   if self._hunger >= 80  then
      self:start_looking_for_seat()
   else
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
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   self._looking_for_seat = false
end

--- Create a pathfinder that will look for unleased chairs in the background
function EatFoodAction:find_seat()
   assert(not self._pathfinder)

   local filter_fn = function(item)
      local is_chair = radiant.entities.get_entity_data(item, 'stonehearth:chair')
      local lease = item:get_component('stonehearth:lease_component')
      if is_chair ~= nil and lease ~= nil then
         return lease:can_acquire_lease(self._entity)
      else
         return false
      end
   end

   local solved_cb = function(path)
      local seat = path:get_destination()
      if seat:add_component('stonehearth:lease_component'):try_to_acquire_lease(self._entity) then 
         self._seat = seat
         self._path_to_seat = path
         self:stop_looking_for_seat()
      end
   end

   radiant.log.info('%s creating pathfinder to find a seat', tostring(self._entity));
   local desc = string.format('finding a seat for %s', tostring(self._entity))
   self._pathfinder = radiant.pathfinder.create_path_finder(desc)
                        :set_source(self._entity)
                        :set_filter_fn(filter_fn)
                        :set_solved_cb(solved_cb)
                        :find_closest_dst()

   assert(self._pathfinder)
end

--- Eat the food we're holding in our hands
--  If we're very hungry, bolt it down nearby, even if we've got a path to a chair.
--  As a result, we'll get less full from the food.  
--  If we're moderately hungry, and have a chair, go there and eat the food. This 
--  grants a bonus to how full we feel after eating. 
--  If we're moderately hungry and don't have a chair, find a random location at
--  moderate distance and eat leisurely. This has no effect on satiation. 
function EatFoodAction:run(ai, entity, food)
   local attributes_component = entity:add_component('stonehearth:attributes')
   local hunger = attributes_component:get_attribute('hunger')
   local entity_loc = entity:get_component('mob'):get_world_grid_location()
   local satisfaction = food:get_component('stonehearth:attributes'):get_attribute('sates_hunger')
   local consumption_text = ''
   local worker_name = radiant.entities.get_display_name(entity)
   self._food = food

   if hunger >= 120 then
      local x = math.random(0 - CLOSE_DISTANCE, CLOSE_DISTANCE)
      local z = math.random(0 - CLOSE_DISTANCE, CLOSE_DISTANCE)
      ai:execute('stonehearth:goto_location', Point3(entity_loc.x + x, entity_loc.y, entity_loc.z + z))

      radiant.entities.set_posture(self._entity, 'sitting')
      ai:execute('stonehearth:run_effect', 'sit_on_ground')
      satisfaction = satisfaction * HURRIED_MODIFIER
      consumption_text = radiant.entities.get_entity_data(food, 'stonehearth:message_starved')

   elseif hunger >= 80 then

      if self._seat then
         ai:execute('stonehearth:goto_entity', self._seat)
         self:_register_for_chair_events()

         
         self._pre_eating_location = radiant.entities.get_location_aligned(self._entity)
         radiant.entities.move_to(self._entity, radiant.entities.get_location_aligned(self._seat))
         radiant.entities.set_posture(self._entity, 'sitting_on_chair')

         local angle  = self._seat:get_component('mob'):get_rotation()
         self._entity:get_component('mob'):turn_to_quaternion(angle)
         
         --TODO: alter the chair modifier depending on distance from table, kind of chair
         satisfaction = satisfaction * CHAIR_MODIFIER
         consumption_text = radiant.entities.get_entity_data(food, 'stonehearth:message_normal')

      else
         --We don't have a path or didn't get the lease. Look for any place nearby
         local x = math.random(0 - FAR_DISTANCE, FAR_DISTANCE)
         local z = math.random(0 - FAR_DISTANCE, FAR_DISTANCE)
         ai:execute('stonehearth:goto_location', Point3(entity_loc.x + x, entity_loc.y, entity_loc.z + z))
         
         radiant.entities.set_posture(self._entity, 'sitting')
         ai:execute('stonehearth:run_effect', 'sit_on_ground')
         consumption_text = radiant.entities.get_entity_data(food, 'stonehearth:message_floor')
      end
   end

   --Eat. Should vary based on the postures set above
   for i = 1, 3 do
      ai:execute('stonehearth:run_effect', 'eat')
   end

   --Decrement hunger by amount relevant to the food type
   local entity_attribute_comp = entity:get_component('stonehearth:attributes')
   local hunger = entity_attribute_comp:get_attribute('hunger')
   entity_attribute_comp:set_attribute('hunger', hunger - satisfaction)

   -- Log successful consumption
   -- TODO: move to personality service
   event_service:add_entry(worker_name .. ': ' .. consumption_text .. '(+' .. satisfaction .. ')')

   --Log in personal event log
   local activity_name = radiant.entities.get_entity_data(food, 'stonehearth:activity_name')
   radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                          {entity = entity, description = activity_name})
end

--- Make sure we hear if the chair is moved while we're eating
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

--- If we reserved the chair to eat on, release the reservation
function EatFoodAction:_release_seat_reservation()
   if self._seat then
      local seat_lease = self._seat:get_component('stonehearth:lease_component')
      seat_lease:release_lease(self._entity)
      self._seat = nil
      self._path_to_seat = nil
   end
end

--- When done with action (either on success or interrupt) clean up.  
-- Destroy the food and clean up the promises.
-- Note: We don't stop the pf etc here because the only
-- reason to stop looking for somewhere to eat is
-- because we're no longer hungry
function EatFoodAction:stop()
   --Drop carrying and destroy
   radiant.entities.drop_carrying_on_ground(self._entity, 
      self._entity:get_component('mob'):get_world_grid_location())
   radiant.entities.destroy_entity(self._food)

   if self._chair_moved_promise then
      self._chair_moved_promise:destroy()
      self._chair_moved_promise = nil
   end

   self:_release_seat_reservation()

   --Whatever posture we had going in (sitting, sitting on chair, etc) should be unset now. 
   local posture = radiant.entities.get_posture(self._entity)
   radiant.entities.unset_posture(self._entity, posture)
   radiant.entities.unset_posture(self._entity, 'sitting_on_chair')

   --move off the chair if necessary
   if self._pre_eating_location then
      radiant.entities.move_to(self._entity, self._pre_eating_location)
      self._pre_eating_location = nil
   end
end

return EatFoodAction
