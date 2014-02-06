local TaskQueue = class()

function TaskQueue:__init(name)
   self._name = name
   self._tasks = {}
   self._count = 0
end

function TaskQueue:remove(id)
   if self._tasks[id] then
      self._log:debug('removing task %d from %s list', id, self._name)
      self._count = self._count - 1
      self._tasks[id] = nil
   end
end

function TaskQueue:add(id, task)
   if not self._tasks[id] then
      self._log:debug('adding task %d from %s list', id, self._name)
      self._count = self._count + 1
      self._tasks[id] = task
   else
      assert(self._tasks[id] == task)
   end
end

function TaskQueue:contains(id)
   return self._tasks[id] ~= nil
end

function TaskQueue:count()
   return self._count
end

return TaskQueue

