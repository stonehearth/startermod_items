local RunTaskAction = require 'services.tasks.run_task_action'

local NEXT_TASK_ID = 1
local STARTED = 'started'
local STOPPED = 'stopped'
local COMPLETED = 'completed'

local INFINITE = 999999 -- infinite enough?

local Task = class()

function Task:__init(scheduler, activity)
   self._scheduler = scheduler
   self._activity = activity
     
   self._id = NEXT_TASK_ID
   NEXT_TASK_ID = NEXT_TASK_ID + 1
   self._log = radiant.log.create_logger('tasks', 'task:'..tostring(self._id))   
   self._log:debug('creating new task for %s', scheduler:get_name())
   self._commited = false
   self._name = activity.name .. ' task'
   self._complete_count = 0
   self._active_count = 0
   self._running_actions = {}
   self._repeating_actions = {}
   self._priority = 2
   self._max_workers = INFINITE
   self:_set_state(STOPPED)
end

function Task:destroy()
   if self._state ~= COMPLETED then
      self._scheduler:_decommit_task(self)
      self:_set_state(COMPLETED)
   end
end

function Task:get_id()
   return self._id
end

function Task:get_activity()
   return self._activity
end

function Task:get_name()
   return self._name
end

function Task:get_priority()
   return self._priority
end

function Task:get_state()
   return self._state
end

function Task:get_args()
   return self._activity[2] or {}
end

function Task:set_name(format, ...)
   self._name = string.format(format, ...)
   return self
end

function Task:set_priority(priority)
   assert(self._state ~= 'started')
   self._priority = priority
   return self
end

function Task:get_action_ctor()
   return self._action_ctor
end

function Task:set_repeat_timeout(time_ms)
   self._repeat_timeout = time_ms
   return self
end

function Task:set_max_workers(max_workers)
   assert(self._state ~= 'started')
   self._max_workers = max_workers
   return self
end

function Task:once()
   return self:times(1)
end

function Task:times(n)
   assert(self._state ~= 'started')
   self._times = n
   return self
end

function Task:start()
   if not self._commited then
      local activity = self._scheduler:get_activity()
      self._action_ctor = {
         name = self._name .. ' action',
         does = activity.name,
         args = activity.args,
         create_action = function(_, ai_component, entity, injecting_entity)
            local action = RunTaskAction(self)
            action.name = self._name .. ' action'
            action.does = activity.name
            action.args = activity.args
            action.priority = self._priority
            return action
         end,
      }
      self._scheduler:_commit_task(self)
      self._commited = true
   end
   self._active_count = 0
   self._complete_count = 0
   self:_set_state(STARTED)
   return self
end

function Task:stop()
   assert(self._activity, 'must call execute before stop')
   self:_set_state(STOPPED)
   return self
end

function Task:_set_state(state)
   self._log:spam('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)
   
   self._state = state
   radiant.events.trigger(self, self._state, self)
   radiant.events.trigger(self, 'state_changed', self, self._state)
end

function Task:_get_active_action_count()
   local count = 0
   for _, _ in pairs(self._running_actions) do
      count = count + 1
   end
   for _, _ in pairs(self._repeating_actions) do
      count = count + 1
   end   
   return count
end

function Task:_is_work_finished()
   -- repeat forever if the task owner hasn't called once() or times()
   if not self._times then
      return false
   end
   -- the amount of work left is just the number of times we're supposed to run
   -- minus the number of things actually done minus the number of people attempting
   -- to complete the task.  if that's 0, there's nothing left for anyone else.
   local active_action_count = self:_get_active_action_count()
   local work_remaining = self._times - self._complete_count - active_action_count
   return work_remaining == 0
end

function Task:_is_work_available()
   -- there's no work avaiable if we've already hit the cap of max workers allowed
   -- to start the task.
   local active_action_count = self:_get_active_action_count()
   if active_action_count >= self._max_workers then
      return false
   end

   -- finally, if there's just nothing to do, there's just nothing to do.
   return not self:_is_work_finished()
end

function Task:_get_next_repeat_timeout()
   if self._repeat_timeout then
      return radiant.gamestate.now() +  self._repeat_timeout
   end
   return nil
end

function Task:_check_repeat_timeout(action)
   local timeout = self._repeating_actions[action]

   if timeout and timeout <= radiant.gamestate.now() then
      
      local work_was_available = self:_is_work_available()
      self._repeating_actions[action] = nil
      local work_is_available = self:_is_work_available()

      if work_is_available and not work_was_available then
         radiant.events.trigger(self, 'work_available', self, true)
      end
   end
end

function Task:_action_is_active(action)
   return self._running_actions[action] or self._repeating_actions[action]
end

function Task:__action_can_start(action)
   -- if we're done, we're done.  that's it!
   if self:_is_work_finished() then
      return false
   end

   -- actions that are already runnign (or we've just told to run) are of course
   -- allowed to start.
   if self:_action_is_active(action) then
      return true
   end

   -- otherwins, only start when the task is active and there's stuff to do.
   return self._state == 'started' and self:_is_work_available()
end

function Task:__action_completed(action)
   assert(self._running_actions[action])
   self._complete_count = self._complete_count + 1

   -- update the running actions map to contain the time when we'll release the
   -- repeat reservation.  this may be nil if the task owner hasn't set one
   local timeout = self:_get_next_repeat_timeout()
   if timeout then
      self._repeating_actions[action] = timeout
      radiant.set_timer(timeout, function()
            self:_check_repeat_timeout(action)
         end)
   end
end

-- this protects races from many actions simultaneously trying to start at
-- the same time.
function Task:__action_try_start(action)
   -- if the work "went away" before the call to __action_can_start and now, reject
   -- the request.  this usually happens because some other action ran in and grabbed
   -- the work item before you could.
   if not self:_is_work_available() then
      return false
   end
   
   self._running_actions[action] = true
   
   -- if this is the last guy to get a job, signal everyone else to let them know
   -- that they don't have a shot (this may trigger them to stop_thinking)
   if not self:_is_work_available() then
      radiant.events.trigger(self, 'work_available', self, false)
   end
   return true
end

function Task:__action_stopped(action)
   self._running_actions[action] = nil

   if self:_is_work_available() then
      radiant.events.trigger(self, 'work_available', self, true)
   end

   if self:_is_work_finished() then
      self._log:debug('task reached max number of completions (%d).  stopping and completing!', self._times)
      self:stop()
      self:destroy()
   end  
end

return Task
