local Task = class()

local NEXT_TASK_ID = 1
local STARTED = 'started'
local STOPPED = 'stopped'
local COMPLETED = 'completed'

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
   self._running_actions = {}
   self._running_actions_count = 0
   self:_set_state(STOPPED)
end

function Task:destroy()
   if self._state ~= COMPLETED then
      self:_set_state(COMPLETED)
      self._scheduler:_decommit_task(self)
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
   return 3
end

function Task:get_state()
   return self._state
end

function Task:get_args()
   return self._activity[2] or {}
end

function Task:set_name(name)
   self._name = name
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
      self._scheduler:_commit_task(self)
      self._commited = true
   end
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

function Task:_is_work_available()
   if not self._times then
      return true
   end
   local work_remaining = self._times - self._complete_count - self._running_actions_count
   return work_remaining > 0
end

function Task:_action_is_running(action)
   return self._running_actions[action] ~= nil
end

function Task:__action_can_start()
   if self:_action_is_running() then
      return true
   end
   return self._state == 'started' and self:_is_work_available()
end

function Task:__action_completed(action)
   self._complete_count = self._complete_count + 1
   if not self:_is_work_available() then
      self._log:debug('task reached max number of completions (%d).  stopping and completing!', self._times)
      self:stop()
      self._ultimate_running_action = action -- 'ultimate' in the sense that it's the last action that will ever run
   end
end

function Task:__action_try_start(action)
   if not self:_is_work_available() then
      return false
   end
   
   if not self:_action_is_running() then
      self._running_actions[action] = true
      self._running_actions_count = self._running_actions_count + 1
   end
   
   if not self:_is_work_available() then
      radiant.events.trigger(self, 'work_available', self, false)
   end
   return true
end


function Task:__action_stopped(action)
   if self:_action_is_running(action) then
      self._running_actions[action] = nil
      self._running_actions_count = self._running_actions_count - 1
   end
   if self:_is_work_available() then
      radiant.events.trigger(self, 'work_available', self, true)
   end
   if self._ultimate_running_action then
      self._log:spam('destroying task, since last possible action has stopped')
      self._ultimate_running_action = nil
      self:destroy()
   end
end

return Task
