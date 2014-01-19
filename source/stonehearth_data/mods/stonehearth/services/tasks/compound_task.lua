
local CompoundTask = class()
function CompoundTask:__init(scheduler, compound_task_ctor, activity)
   self._scheduler = scheduler
   self._activity = activity
   self._compound_task = compound_task_ctor()
end

function CompoundTask:set_name(format, ...)
   self._name = string.format(format, ...)
end

function CompoundTask:once()
   return self
end

function CompoundTask:destroy()
   self._co = nil
   if self._compound_task.destroy then
      self._compound_task:destroy()
   end
end

function CompoundTask:start()
   assert(not self._co)
   
   local args = self._activity.args
   self._co = coroutine.create(function()
         if self._compound_task.start then
            self._compound_task:start(self, args)
         end
         self._compound_task:run(self, args)
         if self._compound_task.stop then
            self._compound_task:stop(self, args)
         end
         radiant.events.trigger(self, 'completed')
      end)
   coroutine.resume(self._co)
   return self
end

function CompoundTask:run_task(activity, args)
   assert(self._co)

   local finished = false

   local task = self._scheduler:create_task(activity, args)
                       :once()
                      
   radiant.events.listen(task, 'completed', function()
         finished = true
         coroutine.resume(self._co)
         return radiant.events.UNLISTEN
      end)

   task:start()      
   while not finished do
      coroutine.yield()
   end
   return task
end

return CompoundTask
