local Task = class()

local NEXT_TASK_ID = 1
local STARTED = 'started'
local STOPPED = 'stopped'

function Task:__init(scheduler, activity)
   self._scheduler = scheduler
   self._activity = activity
   self._args = { select(2, unpack(activity)) }
     
   self._id = NEXT_TASK_ID
   NEXT_TASK_ID = NEXT_TASK_ID + 1
   self._log = radiant.log.create_logger('tasks', 'task:'..tostring(self._id))   
   self._log:debug('creating new task for %s', scheduler:get_name())
   self._commited = false
   self._name = activity[1] .. ' task'
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
   return self._args
end

function Task:set_name(name)
   self._name = name
   return self
end

function Task:start()
   if not self._commited then
      self._scheduler:_commit_task(self)
      self._commited = true
   end
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

return Task
