local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local RunTaskAction = require 'services.tasks.run_task_action'

local NEXT_TASK_ID = 1
local STARTED = 'started'
local STOPPED = 'stopped'
local COMPLETED = 'completed'

local INFINITE = 999999 -- infinite enough?

local Task = class()

function Task:__init(task_group, activity)
   self._task_group = task_group
   self._activity = activity
     
   self._id = NEXT_TASK_ID
   NEXT_TASK_ID = NEXT_TASK_ID + 1
   self._log = radiant.log.create_logger('task', 'task:'..tostring(self._id))   
   self._log:debug('creating new task for %s', task_group:get_name())
   
   self:_set_state(STOPPED)
   self:set_name(activity.name)
   self._complete_count = 0
   self._running_actions = {}
   self._entity_effect_names = {}
   self._effects = {}
   self._priority = 1
   self._max_workers = INFINITE
   self._complete_count = 0
end

function Task:destroy()
   if self._task_group then
      self:stop()
      self._task_group:_on_task_destroy(self)
      self._task_group = nil
      self:_set_state(COMPLETED)
   end
end

function Task:get_name()
   return self._name
end

function Task:set_name(format, ...)
   assert(self._state == STOPPED)
   self._name = '[' .. self._id .. ':' .. string.format(format, ...) .. ']'
   return self
end

function Task:set_source(source)
   assert(self._state == STOPPED)
   self._source = source
   return self
end

function Task:set_priority(priority)
   assert(self._state == STOPPED)
   self._priority = priority
   return self
end

function Task:set_max_workers(max_workers)
   assert(self._state == STOPPED)
   self._max_workers = max_workers
   return self
end

function Task:add_entity_effect(entity, effect_name)
   assert(self._state == STOPPED)
   self._entity_effect_names[entity] = effect_name
   return self
end

function Task:once()
   assert(self._state == STOPPED)
   return self:times(1)
end

function Task:times(n)
   assert(self._state == STOPPED)
   self._times = n
   return self
end

function Task:start()  
   assert(self._state == STOPPED)
   self:_set_state(STARTED)

   self:_create_action()
   self:_create_entity_effects()

   -- ask the task group to start feeding us workers.  we'll inject
   -- the an action to perform this task into these workers as they're
   -- fed to us.
   self._task_group:_start_feeding_task(self)
   return self
end

function Task:stop()
   if self._state ~= STOPPED then
      self:_stop()
      self:_set_state(STOPPED)
   end
   return self
end

function Task:wait()
   local thread = stonehearth.threads:get_current_thread()
   assert(thread, 'no thread running in Task:wait()')

   if self._state ~= COMPLETED then
      radiant.events.listen(self, COMPLETED, function()
            thread:resume()
            return radiant.events.UNLISTEN
         end)
      
      thread:suspend()
      assert(self._state == COMPLETED)
   end

   return self._complete_count == self._times
end

function Task:_create_entity_effects()
   for entity, name in pairs(self._entity_effect_names) do 
      local effect = radiant.effects.run_effect(entity, name)
      table.insert(self._effects, effect)
   end
end

function Task:_destroy_entity_effects()
   for _, effect in ipairs(self._effects) do
      effect:stop();
   end
   self._effects = {}
end

function Task:_create_action()
   if not self._action_ctor then
      local activity = self._task_group:get_activity()
      self._action_ctor = {
         name = self._name,
         does = activity.name,
         args = activity.args,
         create_action = function(_, ai_component, entity, injecting_entity)
            local action = RunTaskAction(self, self._activity)
            action.name = self._name .. ' run task action'
            action.does = activity.name
            action.args = activity.args
            action.priority = self._priority
            return action
         end,
      }
   end
end

function Task:_stop()
   -- stop asking for workers.
   self._task_group:_stop_feeding_task(self)
   self:_destroy_entity_effects()
end

function Task:_feed_worker(worker)
   stonehearth.ai:add_custom_action(worker, self._action_ctor)
end

function Task:_unfeed_worker(worker)
   stonehearth.ai:remove_custom_action(worker, self._action_ctor)
end

function Task:_estimate_task_distance(worker)
   local distance = 0
   local source_location

   local worker_location = radiant.entities.get_world_grid_location(worker)
   if radiant.util.is_a(self._source, Point3) then
      source_location = self._source
   end

   if radiant.util.is_a(self._source, Entity) then
      -- try the destination...
      local destination = self._source:get_component('destination')
      if destination then
         local adjacent = destination:get_adjacent()
         if adjacent then
            local rgn = adjacent:get()
            if not rgn:empty() then
               -- great!  find the closest point..
               local origin = radiant.entities.get_world_grid_location(self._source)
               source_location = rgn:get_closest_point(worker_location) + origin
            end
         end
      end
      if not source_location then
         -- no luck?  use the origin
         source_location = radiant.entities.get_world_grid_location(self._source)
      end
   end

   if source_location then
      -- distance between the worker and the source point
      distance = worker_location:distance_to(source_location)
   end
   return distance
end

function Task:_get_fitness(worker)
   return self._priority, self:_estimate_task_distance(worker)
end

function Task:_set_state(state)
   self._log:spam('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)
   
   self._state = state

   -- trigger out state change.  this may cause any number of RunActionTasks
   -- to start or stop thinking.
   radiant.events.trigger_async(self, self._state, self)
end

function Task:_get_active_action_count()
   local count = 0
   for _, _ in pairs(self._running_actions) do
      count = count + 1
   end
   return count
end

function Task:_is_work_finished()
   -- repeat forever if the task owner hasn't called once() or times()
   if not self._times then
      return false
   end
   return self._times - self._complete_count <= 0
end

function Task:_is_work_available()
   -- finally, if there's just nothing to do, there's just nothing to do.
   if self:_is_work_finished() then
      return false
   end

   -- there's no work avaiable if we've already hit the cap of max workers allowed
   -- to start the task.
   local active_action_count = self:_get_active_action_count()
   if active_action_count >= self._max_workers then
      return false
   end

   -- the amount of work left is just the number of times we're supposed to run
   -- minus the number of things actually done minus the number of people attempting
   -- to complete the task.  if that's 0, there's nothing left for anyone else.
   if not self._times then
      return true
   end

   local active_action_count = self:_get_active_action_count()
   local work_remaining = self._times - self._complete_count - active_action_count
   return work_remaining > 0
end

function Task:_action_is_active(action)
   return self._running_actions[action]
end

function Task:_log_state(tag)
   if self._log:is_enabled(radiant.log.DETAIL) then
      local running_count = 0
      for _, _ in pairs(self._running_actions) do
         running_count = running_count + 1
      end
      local work_available = self:_is_work_available()
      self._log:detail('%s [state:%s running:%d completed:%d total:%s available:%s]',
                       tag, self._state, running_count, self._complete_count, tostring(self._times),
                       tostring(work_available))
   end
end

function Task:__action_try_start_thinking(action)
   self:_log_state('entering __action_try_start_thinking')

   -- actions that are already running (or we've just told to run) are of course
   -- allowed to start.
   if self:_action_is_active(action) then
      self._log:detail('action is active.  can start!')
      return true
   end

   -- otherwise, only start when the task is active and there's stuff to do.
   local can_start = self._state == STARTED and self:_is_work_available()
   self._log:detail('can start? %s', tostring(can_start))
   return can_start
end

-- this protects races from many actions simultaneously trying to start at
-- the same time.
function Task:__action_try_start(action)
   self:_log_state('entering __action_try_start')

   -- if the work "went away" before the call to __action_can_start and now, reject
   -- the request.  this usually happens because some other action ran in and grabbed
   -- the work item before you could.
   if not self:_is_work_available() then
      self:_log_state('cannot start.  no work available.')
      return false
   end
   
   self._running_actions[action] = true
   
   -- if this is the last guy to get a job, signal everyone else to let them know
   -- that they don't have a shot (this may trigger them to stop_thinking)
   if not self:_is_work_available() then
      radiant.events.trigger_async(self, 'work_available', self, false)
      self._task_group:_stop_feeding_task(self)
   end
   self._task_group:_notify_worker_started_task(self, action:get_entity())

   self:_log_state('exiting __action_try_start')
   return true
end

function Task:__action_completed(action)
   self:_log_state('entering __action_completed')

   assert(self._running_actions[action])
   self._complete_count = self._complete_count + 1

   self:_log_state('exiting __action_completed')
end

function Task:__action_destroyed(action)
   self:_log_state('entering __action_destroyed')
   if self._task_group then
      assert(not self._running_actions[action])
   end
   self:_log_state('exiting __action_destroyed')
end

function Task:__action_stopped(action)
   self:_log_state('entering __action_can_stopped')

   if self._task_group then
      local work_was_available = self:_is_work_available()
      self._running_actions[action] = nil
      local work_is_available = self:_is_work_available()

      if work_is_available and not work_was_available then
         radiant.events.trigger_async(self, 'work_available', self, true)
         self._task_group:_start_feeding_task(self)
      end

      self._task_group:_notify_worker_stopped_task(self, action:get_entity())
      self:_unfeed_worker(action:get_entity():get_id())

      if self:_is_work_finished() then
         self._log:debug('task reached max number of completions (%d).  stopping and completing!', self._times)
         self:destroy()
      end
   end
   
   self:_log_state('exiting __action_can_stopped')
end

return Task
