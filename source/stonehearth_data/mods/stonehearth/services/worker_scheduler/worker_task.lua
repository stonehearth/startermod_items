local Color4 = _radiant.csg.Color4
local WorkerTask = class()
local priorities = require('constants').priorities.worker_task
local log = radiant.log.create_logger('worker')

local next_task_id = 1
function WorkerTask:__init(name, scheduler)
   self.id = next_task_id
   next_task_id = next_task_id + 1
   
   self._name = string.format('(worker_task %d %s)', self.id, name)
   self._scheduler = scheduler
   self._pathfinders = {}
   self._destinations = {}
   self._priority = priorities.DEFAULT
   self._debug_color = Color4(0, 255, 0, 128)
end

function WorkerTask:destroy()
   self._running = false
   self._scheduler:remove_worker_task(self)
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
   for _, pf in pairs(self._pathfinders) do
      pf:stop()
   end
   
   self._finish_fn = nil   
   self._get_action_fn = nil
   self._worker_filter_fn = nil
   self._work_object_filter_fn = nil
end

function WorkerTask:get_name()
   return self._name
end

function WorkerTask:set_debug_color(color)
   self._debug_color = color
   for worker_id, pf in pairs(self._pathfinders) do
      pf:set_debug_color(color)
   end
end

function WorkerTask:set_priority(priority)
   self._priority = priority
   return self
end

function WorkerTask:set_action_fn(fn)
   self._get_action_fn = fn
   return self
end

function WorkerTask:set_finish_fn(fn)
   self._finish_fn = fn
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

   self._work_object_filter_fn = filter_fn
   if filter_fn then
      local on_added = function(id, entity)
         if filter_fn(entity) then
            self:add_work_object(entity)
         end
      end
      local on_removed = function(id)
         self:remove_work_object(id)
      end
      self._promise = radiant.terrain.trace_world_entities(self._name, on_added, on_removed)
   end
   return self
end

function WorkerTask:add_work_object(dst)
   local ok = not self._work_object_filter_fn or self._work_object_filter_fn(dst)
   if ok then
      log:debug('%s adding work object %s in add_work_object()', self._name, dst)
      self._destinations[dst:get_id()] = dst
      for worker_id, pf in pairs(self._pathfinders) do
         pf:add_destination(dst)
      end
      return self
   else
      log:debug('%s ignoring non-suitable %s in add_work_object()', self._name, dst)
   end
end

function WorkerTask:remove_work_object(id)
   self._destinations[id] = nil
   for worker_id, pf in pairs(self._pathfinders) do
      pf:remove_destination(id)
   end
   return self
end

function WorkerTask:start()
   if not self._running then
      self._running = true
      self._scheduler:_start_worker_task(self)
   end
   return self
end

function WorkerTask:stop()
   if self._running then
      self._running = false
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

function WorkerTask:is_running()
   return self._running
end

--- Consider Worker for task
-- Test if the worker meets the filter function criteria. If so, activate pathfinder
function WorkerTask:_consider_worker(worker)
   assert(self._running, string.format('%s cannot consider worker while not running', self._name))
   assert(self._get_action_fn, string.format('no action function set for WorkerTask %s', self._name))
   assert(self._worker_filter_fn, string.format('no worker filter function set for WorkerTask %s', self._name))

   local worker_id = worker:get_id()
   if not self._pathfinders[worker_id] then
      if self._worker_filter_fn(worker) then
         -- create a new pathfinder for this worker
         local name = string.format('%s for worker %d', self._name, worker_id)
         local pf = radiant.pathfinder.create_path_finder(name)
                                       :set_source(worker)
                                       :set_debug_color(self._debug_color)
                                       
         -- forward all the valid destinations for this task to the new
         -- pathfinder
         for id, dst in pairs(self._destinations) do
            pf:add_destination(dst)
         end
         
         -- in the pathfinder, setup a solution function to dispath the
         -- task to the worker
         pf:set_solved_cb(function(path)
            self:_dispatch_solution(path)
         end)
         
         self._pathfinders[worker_id] = pf
      else
         self:_remove_worker(worker_id)
      end
   end
end

function WorkerTask:_dispatch_solution(path)
   local worker_id = path:get_source():get_id()
   local destination_id = path:get_destination():get_id()
   local pf = self._pathfinders[worker_id]

   local action = { self._get_action_fn(path) }
   local finish_fn = function(success)
      if self._finish_fn then
         self._finish_fn(success, action)
      else
         log:debug('%s no finish function set by client!  ignoring', self._name)
      end
   end
   pf:stop()
   self._scheduler:dispatch_solution(self._priority, worker_id, destination_id, action, finish_fn)
end

return WorkerTask
