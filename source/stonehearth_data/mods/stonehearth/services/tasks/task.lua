local Task = class()

local NEXT_TASK_ID = 1
local STARTED = 'started'
local STOPPED = 'stopped'

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
   self:_set_state(STOPPED)
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

function Task:__completed()
   self._complete_count = self._complete_count + 1
   if self._complete_count == self._times then
      self._log:debug('task reached max number of completions (%d).  stopping', self._times)
      self:stop()
   end
end

return Task
