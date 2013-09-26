local WorkerTask = class()

local next_task_id = 1
function WorkerTask:__init(name, scheduler)
   self.id = next_task_id
   next_task_id = next_task_id + 1

   self.name = string.format('worker_task (%d): %s', self.id, name)
   self._scheduler = scheduler
   self._pathfinder = radiant.pathfinder.create_multi_path_finder(name)

   --self._run_once = false    --by default, all tasks go to all eligible workers and are always around
end

function WorkerTask:destroy()
   self.running = false
   self._scheduler._remove_worker_task(self)
end

--TODO: figure out if this is a good idea later.
--- Run_once means that once one person can do the task, it is destroyed.
-- Set when the task is created, before it has started.
-- TODO: distinguish between just 1 person and just once for all people.
-- TODO: for example, if the task was to chop down a tree, and tree is gone... how to tell task?
-- @param run_once True if you want the task to run with just one person, false if n people can do it n times
--function WorkerTask:set_run_once(run_once)
--   self._run_once = run_once
--   return self
--end

--function WorkerTask:get_run_once()
--   return self._run_once
--end

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
   -- Does dst still exist? If not, destroy this
   if dst then
      self._pathfinder:add_destination(dst)
   else
      self.destroy()
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

function WorkerTask:_remove_worker(worker)
   self._pathfinder:remove_entity(worker)
end

--- Consider Worker for task
-- Test if the worker meets the filter function criteria. If so, activate pathfinder
-- If it is a run_once task, then immediately stop the task.
-- When the other pathfinders return, DO NOT dispatch those workers to this solution.
function WorkerTask:_consider_worker(worker)
   assert(self.running, string.format('%s cannot consider worker while not running', self.name))
   assert(self._get_action_fn, string.format('no action function set for WorkerTask %s', self.name))
   assert(self._worker_filter_fn, string.format('no worker filter function set for WorkerTask %s', self.name))

   local solved_cb = function(path)
      local action = { self._get_action_fn(path) }

      -- The pathfinder has returned, but before we dispatch we should test a few things.
      -- First, is the target entity still around? If not, kill this task.
      local item = path:get_destination()

      if not item then
         self.destroy()
         return
      end

      --TODO: add this?
      -- Is this a run_once task? If so, is it still running? If so, you are
      -- the first worker to figure out how to proceed. Congrats!
      --if self._run_once and self.running then
         -- For now, close the door behind you by stopping the task (ie setting it to not running).
         -- TODO: Ideally, it would be nice to destroy the task completely when it's done
         -- Other people who find the task not running should just skip this step.
      --   self:_dispatch_solution(action, path)
         --self:stop()
      --   self.running = false
      --end

      --If it's not a run-once task, then always dispatch
      --if not self._run_once then
         self:_dispatch_solution(action, path)
      --end
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
