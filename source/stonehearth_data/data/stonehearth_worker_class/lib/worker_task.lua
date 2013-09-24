local WorkerTask = class()

local next_task_id = 1
function WorkerTask:__init(name, scheduler)
   self.id = next_task_id
   next_task_id = next_task_id + 1
   
   self.name = string.format('worker_task (%d): %s', self.id, name)
   self._scheduler = scheduler
   self._pathfinder = radiant.pathfinder.create_multi_path_finder(name)
end

function WorkerTask:destroy()
   self.running = false
   self._scheduler._remove_worker_task(self)   
end

function WorkerTask:set_action_fn(fn)
   self._get_action_fn = fn
   return self
end

function WorkerTask:set_action(action_name)
   self:set_action_fn(function (path)
         return action_name, path
      end)
   return self
end

function WorkerTask:set_worker_filter_fn(filter_fn)
   self._worker_filter_fn = filter_fn
   return self
end

function WorkerTask:set_work_object_filter_fn(filter_fn)
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end

   if filter_fn then
      local on_added = function(id, entity)
         if filter_fn(entity) then
            self._pathfinder:add_destination(entity)
         end
      end
      local on_removed = function(id)
         self._pathfinder:remove_destination(id)
      end
      self._promise = radiant.terrain.trace_world_entities(self.name, on_added, on_removed)
   end
   return self
end

function WorkerTask:add_work_object(dst)
   self._pathfinder:add_destination(dst)
   return self
end

function WorkerTask:start()
   if not self.running then
      self.running = true
      self._scheduler:_start_worker_task(self)
   end
   return self
end

function WorkerTask:stop()
   if self.running then
      self.running = false
      self._scheduler:_stop_worker_task(self)
   end
   return self
end

function WorkerTask:_remove_worker(id)
   self._pathfinder:remove_entity(id)
end

function WorkerTask:_consider_worker(worker)
   assert(self.running, string.format('%s cannot consider worker while not running', self.name))
   assert(self._get_action_fn, string.format('no action function set for WorkerTask %s', self.name))
   assert(self._worker_filter_fn, string.format('no worker filter function set for WorkerTask %s', self.name))

   local solved_cb = function(path)
      local action = { self._get_action_fn(path) }
      self:_dispatch_solution(action, path)
   end

   if self._worker_filter_fn(worker) then
      self._pathfinder:add_entity(worker, solved_cb, nil)
   else
      self._pathfinder:remove_entity(worker:get_id())
   end
end

function WorkerTask:_dispatch_solution(action, path)
   self._scheduler:_dispatch_solution(action, path)
end

return WorkerTask
