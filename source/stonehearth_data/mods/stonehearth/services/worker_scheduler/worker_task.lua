local Color4 = _radiant.csg.Color4
local WorkerTask = class()


local next_task_id = 1
function WorkerTask:__init(name, scheduler)
   self.id = next_task_id
   next_task_id = next_task_id + 1
   
   self.name = name
   self._scheduler = scheduler
   self._pathfinders = {}
   self._destinations = {}
   self._debug_color = Color4(0, 255, 0, 128)
end

function WorkerTask:destroy()
   self.running = false
   self._scheduler._remove_worker_task(self)   
end

function WorkerTask:set_debug_color(color)
   self._debug_color = color
   for worker_id, pf in pairs(self._pathfinders) do
      pf:set_debug_color(color)
   end
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
            self:add_work_object(entity)
         end
      end
      local on_removed = function(id)
         self:remove_work_object(id)
      end
      self._promise = radiant.terrain.trace_world_entities(self.name, on_added, on_removed)
   end
   return self
end

function WorkerTask:add_work_object(dst)
   self._destinations[dst:get_id()] = dst
   for worker_id, pf in pairs(self._pathfinders) do
      pf:add_destination(dst)
   end
   return self
end

function WorkerTask:remove_work_object(id)
   self._destinations[id] = nil
   for worker_id, pf in pairs(self._pathfinders) do
      pf:remove_destination(id)
   end
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
   local pf = self._pathfinders[id]
   if pf then
      pf:stop()
      self._pathfinders[id] = nil
   end
end

function WorkerTask:_consider_worker(worker)
   assert(self.running, string.format('%s cannot consider worker while not running', self.name))
   assert(self._get_action_fn, string.format('no action function set for WorkerTask %s', self.name))
   assert(self._worker_filter_fn, string.format('no worker filter function set for WorkerTask %s', self.name))

   local id = worker:get_id()
   if not self._pathfinders[id] then
      local solved_cb = function(path)
         local action = { self._get_action_fn(path) }
         self:_dispatch_solution(action, path)
      end

      if self._worker_filter_fn(worker) then
         local name = string.format('%s for worker %d', self.name, id)
         local pf = radiant.pathfinder.create_path_finder(name)
                                       :set_source(worker)
                                       :set_solved_cb(solved_cb)
                                       :set_debug_color(self._debug_color)
         for id, dst in pairs(self._destinations) do
            pf:add_destination(dst)
         end
         self._pathfinders[id] = pf
         pf = pf
      else
         self:_remove_worker(id)
      end
   end
end

function WorkerTask:_dispatch_solution(action, path)
   self._scheduler:_dispatch_solution(action, path)
end

return WorkerTask
