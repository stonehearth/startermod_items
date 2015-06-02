local WeightedSet = require 'lib.algorithms.weighted_set'
local PetFns = require 'ai.actions.pet.pet_fns'

local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('pets')

local PetOrchestrator = class()

function PetOrchestrator:__init()
   self._sight_radius = radiant.util.get_config('sight_radius', 64)
   self._idle_time = 500
   self._follow_time = 1000

   local activities = WeightedSet(rng)
   activities:add(self._run_idle, 20)
   activities:add(self._run_follow_friend, 40)
   activities:add(self._run_play, 40)
   self._activities = activities
end

function PetOrchestrator:run(town, args)
   self._pet = args.entity
   self._task_group = self._pet:add_component('stonehearth:ai')
                                    :get_task_group('stonehearth:ambient_pet_behavior')
   local pet = self._pet
   local player_id = town:get_player_id()
   local activity_fn

   while pet:is_valid() and pet:add_component('unit_info'):get_player_id() == player_id do
      activity_fn = self:_choose_activity()
      activity_fn(self, pet, town)
   end
end

function PetOrchestrator:_run_idle(pet, town)
   log:debug('%s starting _run_idle', tostring(pet))

   local task
   task = self._task_group:create_task('stonehearth:idle')
      :start()

   stonehearth.calendar:set_timer("PetOrchestrator stop idle", self._idle_time, function()
         task:destroy()
      end)

   task:wait()
end

-- Moving this into its own action didn't work. Couldn't get idle to run when close to friend (and nothing to do)
function PetOrchestrator:_run_follow_friend(pet, town)
   log:debug('%s starting _run_follow_friend', tostring(pet))

   local friends, num_friends = PetFns.get_friends_nearby(self._pet, self._sight_radius)

   if num_friends == 0 then
      return
   end

   local roll = rng:get_int(1, num_friends)
   local target = friends[roll]
   local task

   -- follow continuously
   -- will suspend and defer to idle if close to a citizen that is not moving
   local task

   task = self._task_group:create_task('stonehearth:follow_entity', { target = target })
      :start()

   -- stop after _follow_time game seconds
   stonehearth.calendar:set_timer("PetOrchestrator stop follow", self._follow_time, function ()
         task:destroy()
      end)

   task:wait()
end

function PetOrchestrator:_run_play(pet, town)
   log:debug('%s starting _run_play', tostring(pet))

   self._task_group:create_task('stonehearth:pet:play')
      :once()
      :start()
      :wait()
end

function PetOrchestrator:stop()
end

function PetOrchestrator:destroy()
end

-- duration is in game seconds
function PetOrchestrator:_suspend(duration)
   local thread = stonehearth.threads:get_current_thread()

   stonehearth.calendar:set_timer("PetOrchestrator resume", duration,
      function ()
         -- looks like case is handled if thread was already terminated
         thread:resume()
      end
   )

   thread:suspend()
end

function PetOrchestrator:_choose_activity()
   local friends, num_friends = PetFns.get_friends_nearby(self._pet, self._sight_radius)

   if num_friends > 1 then
      -- Several people nearby, it's safe to play! All activities are ok.
      return self._activities:choose_random()
   end

   if num_friends == 1 then
      -- Only one person here. Stay close! Follow your caretaker.
      return self._run_follow_friend
   end

   -- Pet is stranded and doesn't want to play!
   return self._run_idle
end

return PetOrchestrator
