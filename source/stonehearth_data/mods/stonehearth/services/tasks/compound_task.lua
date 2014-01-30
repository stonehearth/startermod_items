
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
   self._thread:terminate()
   self._thread = nil
   if self._compound_task.destroy then
      self._compound_task:destroy()
   end
end

function CompoundTask:start()
   assert(not self._thread)
   
   local args = self._activity.args
   self._thread = stonehearth.threads:create_thread()
   self._thread:set_thread_main(function()
         if self._compound_task.start then
            self._compound_task:start(self, args)
         end
         self._compound_task:run(self, args)
         if self._compound_task.stop then
            self._compound_task:stop(self, args)
         end
         radiant.events.trigger(self, 'completed')
      end)
   self._thread:start()
   return self
end

function CompoundTask:suspend()
   self._thread:suspend()
end

function CompoundTask:resume()
   self._thread:resume()
end

function CompoundTask:execute(activity, args)
   local task = self._scheduler:create_task(activity, args)
                                    :once()
   self:_run(task)
end

function CompoundTask:orchestrate(activity, args)
   local orchestrator = self._scheduler:create_orchestrator(activity, args)
                                    :once()
   self:_run(orchestrator)
end

function CompoundTask:_run(task)
   assert(self._thread)

   local finished = false
                     
   radiant.events.listen(task, 'completed', function()
         finished = true
         self:resume()
         return radiant.events.UNLISTEN
      end)

   task:start()      
   while not finished do
      self:suspend()
   end
   return task
end

return CompoundTask
