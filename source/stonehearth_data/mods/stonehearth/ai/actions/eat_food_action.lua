--[[
   Run to a food dish and serve self from it. 
--]]
radiant.mods.load('stonehearth')
local priorities = require('constants').priorities.needs
local Point3 = _radiant.csg.Point3

local EatFoodAction = class()

EatFoodAction.name = 'eat food'
EatFoodAction.does = 'stonehearth:eat_food'
EatFoodAction.priority = 5  --The minute this is called, it runs. 

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

   if hunger >= 120 then
      --We're starving! It doesn't matter if we have a seat.
      --go a few steps away and run the eating animation while standing
      local x = math.random(-3, 3)
      local z = math.random(-3, 3)
      ai:execute('stonehearth:goto_location', Point3(entity_loc.x + x, entity_loc.y, entity_loc.z + z))
      --TODO: replace with standing/eating animation
      ai:execute('stonehearth:run_effect', 'work')
   elseif hunger >= 80 then
      --if we have a path, grab the seat's lease, goto the seat, sit, and eat
      if self._path_to_seat and self._seat:add_component('stonehearth:lease_component'):try_to_acquire_lease(entity) then 
         --We successfully acquired the lease
         radiant.log.info('leasing %s to %s', tostring(self._seat), tostring(self._entity))
         ai:execute('stonehearth:goto_entity', self._seat)
         --play sitting and eating animation
         ai:execute('stonehearth:run_effect', 'work')
      else
         --We don't have a pth or didn't get the lease. Look for a place 10 units away, sit and eat there
         local x = math.random(-6, 6)
         local z = math.random(-6, 6)
         ai:execute('stonehearth:goto_location', Point3(entity_loc.x + x, entity_loc.y, entity_loc.z + z))
         --TODO: replace with sitting/eating animation
         ai:execute('stonehearth:run_effect', 'work')
      end
   end

   --Decrement hunger by amount relevant to the food type
   local satisfaction = food:get_component('stonehearth:attributes'):get_attribute('sates_hunger')
   local entity_attribute_comp = entity:get_component('stonehearth:attributes')
   local hunger = entity_attribute_comp:get_attribute('hunger')
   entity_attribute_comp:set_attribute('hunger', hunger - satisfaction)

   --Drop carrying and destroy
   radiant.entities.drop_carrying_on_ground(entity, entity:get_component('mob'):get_world_grid_location())
   radiant.entities.destroy_entity(food)
end

function EatFoodAction:stop()
   self:stop_looking_for_seat()
end

return EatFoodAction
