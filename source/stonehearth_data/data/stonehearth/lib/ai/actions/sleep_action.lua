--[[
   Need a more robust implementation later. For now,
   go to sleep at 9pm and wake up at 6am
--]]
radiant.mods.get_singleton('stonehearth') -- make sure it's loaded...

local SleepAction = class()

SleepAction.name = 'stonehearth.actions.sleep'
SleepAction.does = 'stonehearth.activities.top'
SleepAction.priority = 0

function SleepAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity         --the game character
   self._ai = ai

   self._sleepiness = 0

   radiant.events.listen('radiant.events.calendar.hourly', self)
end

--[[
   The sleep action works by first looking for a bed when it's time to go
   to sleep. If a bed is found, we run to the bed and lie down in it. 
   
   If a bed is never found, we will never go to sleep. The idea is that
   some other action will pick up the slack and make the actor go to sleep
   on the ground (not in a bed) from exhaustion.
--]]
SleepAction['radiant.events.calendar.hourly'] = function(self, calendar)
   radiant.log.info('adjusting sleepiness')

   --[[
      Pathfinder callback. When it's time to go to sleep, the pathfinder
      is started to go find a path to our bed. Once the path is found,
      jack up the action's priority so the dude will go find his bed
      and go to sleep.
   --]]
   local solved_cb = function(bed, path)
      self._bed = bed
      self._path_to_bed = path
      self._ai:set_action_priority(self, 100)
   end

   -- if it's after 8pm or before 6am and we're not already asleep got find a bed.
   if calendar.hour > 20 or calendar.hour < 6 then
      if not self._pathfinder and not self._sleeping then
         self:_start_looking_for_bed(solved_cb)
      end
   else 
      -- it's not time to go to sleep. Stop looking for a bed
      if self._pathfinder then
         self:stop()
      end
      self._ai:set_action_priority(self, 0)
   end
end

--[[
   Finds a bed to sleep in, if one is available. Actors "own" individual beds through
   a lease system.
   
   Sleeping in a bed pairs that bed to the actor through a lease. Other actors will
   only look for unleased beds to sleep in. Once an actor has leased a bed, he will
   always go back to that bed when it's time to sleep.
--]]
function SleepAction:_start_looking_for_bed(result_cb)
   assert(not self._pathfinder)

   -- find a bed
   local faction = self._entity:get_component('unit_info'):get_faction()

   -- the bed lease is a required component for the sleeping behavior to work
   local bed_lease = self._entity:get_component('stonehearth_sleep_system:bed_lease')
   if not bed_lease then
      radiant.log.warning('missing bed lease component.  ignoring sleep behavior.')
      return
   end
   
   local bed = bed_lease:get_bed()
   if bed then
      -- we already have a bed assigned to us, so just save the path to the bed once we find it
      local desc = string.format('finding bed for leased bed %s', tostring(self._entity))
      local solved_cb = function(path)
         result_cb(bed, path)
      end
      self._pathfinder = radiant.pathfinder.find_path_to_entity(desc, self._entity, bed, solved_cb)
   else
      -- find a bed and lease it
      local filter_fn = function(item)
         radiant.log.info("looing for a bed")    
         -- xxx: only look for beds compatible with this entities faction
         local bed_component = item:get_component('stonehearth_sleep_system:bed')
         if bed_component ~= nil then
            local owner = bed_component:get_owner()
            return owner == nil
         else
            return false
         end
      end

      -- save the path to the bed once we find it
      local solved_cb = function(path)
         local bed = path:get_destination()
         result_cb(bed, path)
      end

      -- go find the path to the bed
      local desc = string.format('finding bed for %s', tostring(self._entity))
      self._pathfinder = radiant.pathfinder.find_path_to_closest_entity(desc, self._entity, solved_cb, filter_fn)
   end

   assert(self._pathfinder)
end

--[[
   SleepAction only runs once a path to the bed has been found.
   
   SleepAction handles the bookkeeping of the sleep behavior. It tells the pathfinder to
   stop searching for a bed, grabs a lease on the bed if necessary, then kicks off
   stonehearth.activities.sleep_in_bed to handle the specifics of how to go to sleep
   (like which animations to play)
--]]
function SleepAction:run(ai, entity)
   assert(self._pathfinder)   
   assert(self._path_to_bed)

   -- tell the pathfinder to stop searching in the background
   self._pathfinder:stop()
   self._pathfinder = nil

   -- renew our lease on the bed.
   local bed_component = self._bed:get_component('stonehearth_sleep_system:bed')
   local bed_owner = bed_component:get_owner()
   if bed_owner and bed_owner:get_id() ~= entity:get_id() then
      -- xxx. if there's another lease on the bed that doesn't belong to us,
      -- we need to bail.  this can happen if two people both try to sleep in
      -- a bed simultaneously
      assert(false)
   end
   bed_component:lease_bed_to(entity)
   entity:get_component('stonehearth_sleep_system:bed_lease'):set_bed(self._bed)

   -- go to sleep!
   self._sleeping = true
   ai:execute('stonehearth.activities.sleep_in_bed', self._bed, self._path_to_bed)
   self._sleeping = false
end

--[[
   If the action is stopped, stop the pathfinder
--]]
function SleepAction:stop()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   
   self._sleeping = false
end

return SleepAction
