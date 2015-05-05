local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local RunTaskAction = require 'services.server.tasks.run_task_action'

local NEXT_TASK_ID = 1
local STARTED = 'started'
local PAUSED = 'paused'
local COMPLETED = 'completed'
local DESTROYED = 'destroyed'

local INFINITE = 1000000000 -- infinite enough?

local Task = class()

function Task:__init(task_group, activity)
   self._task_group = task_group
   self._activity = activity
     
   self._id = task_group:get_next_task_id()
   self._model = radiant.create_datastore()
   
   self._log = radiant.log.create_logger('task', 'task:'..tostring(self._id))   
   self._log:debug('creating new task for %s', task_group:get_name())
   
   self:_set_state(PAUSED)
   self:set_name(activity.name)
   self._complete_count = 0
   self._running_actions = {}
   self._worker_affinity_timeout = {}
   self._entity_effect_names = {}
   self._effects = {}
   self._priority = 1
   self._max_workers = INFINITE
   self._fed_workers = 0
   self._currently_feeding = false
   self._notify_completed_cbs = {} --called when task is completed
   --TODO: add a set of callbacks for failure state
   self._notify_destroyed_cbs = {} --called when task is destroyed (which happens on success OR failure)
   self._workers_pending_unfeed = {}
end

function Task:destroy()
   if self._state ~= DESTROYED then
      self._log:detail('user initiated destroy (state:%s).', self._state)
      self:_destroy()
   else
      self._log:detail('ignorning redunant call to destroy (state: %s)', self._state)
   end
end

function Task:_destroy()
   assert(self._state ~= DESTROYED)
   self:_set_state(DESTROYED)
   self:_stop_feeding()
   self:_destroy_entity_effects()

   --Fire all the destroyed callbacks
   self:_fire_cbs(DESTROYED, self._notify_destroyed_cbs)

   self._log:detail('notifying task group of destruction')
   self._task_group:_on_task_destroy(self)
   self._task_group = nil

   if self._model then
      self._model:destroy()
      self._model = nil
   end
end

--Fire either the notify_completed or notify_destroyed callbacks
--depending on target state passed in an the callback array passed in
function Task:_fire_cbs(target_state, callbacks)
   assert(self._state == target_state)
   for _, cb in ipairs(callbacks) do
      cb()
   end
end

--Returns true if the task is still going, false otherwise
function Task:is_active()
   return self._state == STARTED or self._state == PAUSED
end

--Fired when the task completes successfully
function Task:notify_completed(cb)
   table.insert(self._notify_completed_cbs, cb)
   return self
end

--Fired when the task is cleaned up, whether the task failed or succeeded
function Task:notify_destroyed(cb)
   table.insert(self._notify_destroyed_cbs, cb)
   return self
end

function Task:get_name()
   return self._name
end

function Task:get_model()
   return self._model
end

function Task:get_id()
   return self._id
end

function Task:get_priority()
   return self._priority
end

function Task:set_affinity_timeout(timeout)
   if timeout ~= nil then
      local calendar_timeout = stonehearth.calendar:realtime_to_calendar_time(timeout)
      self._log:debug('setting affinity timeout to %d calendar seconds (%d realtime seconds)', calendar_timeout, timeout)
      self._affinity_timeout = calendar_timeout
   else
      self._affinity_timeout = nil
   end
   return self
end

function Task:get_affinity_timeout()
   return self._affinity_timeout
end

function Task:check_worker_against_task_affinity(worker)
   if not self._affinity_timeout then
      -- task has not been configured with worker affinity.
      return true
   end

   local now = stonehearth.calendar:get_elapsed_time()
   local expire_time = self._worker_affinity_timeout[worker:get_id()]
   if expire_time then
      if expire_time > now then
         -- worker restarted this task before his affinity timeout expired.  definitely
         -- let him through.
         self._log:debug('%s resuming task %s with %d ticks left', worker, self._name, (expire_time - now))
         return true
      else
         self._log:debug('%s lost affinity to ask task %s with (%d - %d)', worker, self._name, now, expire_time)
      end
   end

   -- count the number of non-expired timeouts we have left...
   local still_bound_count = 0
   for id, expire_time in pairs(self._worker_affinity_timeout) do
      if expire_time < now then
         self._worker_affinity_timeout[id] = nil
         self._log:debug('%s did not resume task %s in time.  breaking affinity', worker, self._name)
      else
         still_bound_count = still_bound_count + 1
      end
   end

   -- if there's room for more workers, we're good.
   return still_bound_count < self._max_workers
end

function Task:set_name(format, ...)
   assert(self._state == PAUSED)
   self._name = '[' .. self._id .. ':' .. string.format(format, ...) .. ']'

   self._model:modify(function(o)
         o.name = self._name
      end)

   return self
end

function Task:set_source(source)
   assert(self._state == PAUSED)
   self._source = source
   self._model:modify(function(o)
         o.source = source
      end)
   return self
end

function Task:set_priority(priority)
   assert(priority)
   assert(self._state == PAUSED)
   self._priority = priority
   return self
end

function Task:set_max_workers(max_workers)
   assert(self._state == PAUSED)
   assert(max_workers > 0, 'Task:set_max_workers() called with non-positive value')
   self._max_workers = max_workers
   return self
end

function Task:add_entity_effect(entity, effect_name)
   assert(self._state == PAUSED)
   self._entity_effect_names[entity] = effect_name
   return self
end

function Task:once()
   assert(self._state == PAUSED)
   return self:times(1)
end

function Task:times(n)
   assert(self._state == PAUSED)
   self._times = n
   return self
end

function Task:start()  
   assert(self._state == PAUSED)
   self:_set_state(STARTED)

   self:_create_action()
   self:_create_entity_effects()

   -- ask the task group to start feeding us workers.  we'll inject
   -- the an action to perform this task into these workers as they're
   -- fed to us.
   self:_start_feeding()
   return self
end

function Task:pause()
   self._log:detail('user initiated pause.')
   if self._state ~= PAUSED then
      self:_stop_feeding()
      self:_destroy_entity_effects()
      self:_set_state(PAUSED)
   end
   return self
end

--TODO: whenever anyone uses this, they have to consider what might happen if the task CANNOT complete
--See te clear_workshop orchestrator. Can that logic be rolled up into this function somehow?
function Task:wait()
   self._log:detail('user initiated wait.')
   local thread = stonehearth.threads:get_current_thread()
   assert(thread, 'no thread running in Task:wait()')

   if self:is_active() then
      local function cb()
         self._completed_listener:destroy()
         self._completed_listener = nil

         self._destroyed_listener:destroy()
         self._destroyed_listener = nil

         thread:resume()
      end
      self._completed_listener = radiant.events.listen(self, COMPLETED, cb)
      self._destroyed_listener = radiant.events.listen(self, DESTROYED, cb)
      
      thread:suspend()
      assert(not self:is_active())
   end

   return self._complete_count == self._times
end

function Task:_create_entity_effects()
   for entity, name in pairs(self._entity_effect_names) do 
      local effect = radiant.effects.run_effect(entity, name)
      table.insert(self._effects, effect)
   end
end

function Task:_destroy_entity_effects()
   for _, effect in ipairs(self._effects) do
      effect:stop();
   end
   self._effects = {}
end

function Task:_create_action()
   if not self._action_ctor then
      local activity = self._task_group:get_activity()
      self._action_ctor = {
         name = self._name,
         does = activity.name,
         args = activity.args,
         create_action = function(_, ai_component, entity, injecting_entity)
            local action = RunTaskAction(self, self._activity)
            action.name = self._name .. ' run task action ' .. stonehearth.ai:format_activity(self._activity)
            action.does = activity.name
            action.args = activity.args
            action.priority = self._priority
            return action
         end,
      }
   end
end

function Task:_feed_worker(worker)
   if not worker or not worker:is_valid() then
      return
   end

   -- if the worker is in the list of workers to remove the run_task_action
   -- from, go ahead and remove it.  this will leave the current action there
   -- undisturbed.  otherwise, inject a new run_task_action into the worker.
   local id = worker:get_id()
   if self._workers_pending_unfeed[id] then
      self._log:debug('removing pending unfeed worker run task action from %s', worker)
      self._workers_pending_unfeed[id] = nil
   else
      self._log:debug('injecting run task action into %s', worker)
      worker:get_component('stonehearth:ai')
               :add_custom_action(self._action_ctor)
   end
   self._fed_workers = self._fed_workers + 1
end

-- remove our action from all the workers which need to be unfed.  we defer
-- these till we're definitely back on the main game loop to avoid the condition
-- described in Thread:interrupt().  This loop is written in such a way that
-- all workers will be processed, even if workers get added or removed during
-- iteration (i.e. don't use pairs!)
function Task:_process_deferred_unfed_workers()
   while true do
      local worker_id = next(self._workers_pending_unfeed)
      if not worker_id then
         break
      end
      local worker = self._workers_pending_unfeed[worker_id]
      self._workers_pending_unfeed[worker_id] = nil
      self._fed_workers = self._fed_workers - 1
      if worker:is_valid() then
         self._log:detail('removing run task action from %s', worker)
         worker:get_component('stonehearth:ai'):remove_custom_action(self._action_ctor)
      end
   end
end

function Task:_unfeed_worker(worker)
   if type(worker) == 'number' then
      worker = radiant.entities.get_entity(worker)
   end
   -- if the worker's not around anymore, go ahead and bail.
   if not worker or not worker:is_valid() then
      return
   end

   -- if we are already planning to unfeed this worker, there's
   -- nothing more that needs to be done
   if self._workers_pending_unfeed[worker:get_id()] then
      return
   end

   -- if the list of unfed workers is empty, schedule a one time callback to
   -- uninject their actions on the main loop, then add the worker to the list.
   if next(self._workers_pending_unfeed) == nil then
      radiant.events.listen_once(radiant, 'stonehearth:gameloop', function()
            self:_process_deferred_unfed_workers()
         end)
   end
   self._log:detail('adding worker %s to pending unfeed list', worker)
   self._workers_pending_unfeed[worker:get_id()] = worker
end

function Task:_estimate_task_distance(worker)
   local distance = 0
   local source_location

   local worker_location = radiant.entities.get_world_grid_location(worker)
   if radiant.util.is_a(self._source, Point3) then
      source_location = self._source
   elseif radiant.util.is_a(self._source, Entity) and self._source:is_valid() then
      -- try the destination...
      local destination = self._source:get_component('destination')
      if destination then
         local adjacent = destination:get_adjacent()
         if adjacent then
            local rgn = adjacent:get()
            if not rgn:empty() then
               -- great!  find the closest point..
               local origin = radiant.entities.get_world_grid_location(self._source)
               source_location = rgn:get_closest_point(worker_location - origin) + origin
            end
         end
      end
      if not source_location then
         -- no luck?  use the origin
         source_location = radiant.entities.get_world_grid_location(self._source)
      end
   end

   if source_location then
      -- distance between the worker and the source point
      distance = worker_location:distance_to(source_location)
   end
   return distance
end

function Task:_get_fitness(worker)
   return self._priority, self:_estimate_task_distance(worker)
end

function Task:_set_state(state)
   self._log:spam('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)
   
   self._state = state

   -- trigger out state change.  this may cause any number of RunActionTasks
   -- to start or stop thinking.
   radiant.events.trigger_async(self, self._state, self)
end

function Task:_get_active_action_count()
   local count = 0
   for _, _ in pairs(self._running_actions) do
      count = count + 1
   end
   return count
end

function Task:_is_work_finished()
   -- repeat forever if the task owner hasn't called once() or times()
   if not self._times then
      return false
   end
   return self._times - self._complete_count <= 0
end

function Task:has_worker_affinity_expired(worker)
   if not self._affinity_timeout then
      -- task has not been configured with worker affinity.
      return true
   end

   local expire_time = self._worker_affinity_timeout[worker:get_id()]
   if not expire_time then
      -- the worker is not in the affinity window
      return true
   end

   local now = stonehearth.calendar:get_elapsed_time()
   if expire_time < now then
      -- the worker hasn't restarted the task before the timeout expired.  bail
      return true
   end

   -- no, this worker is not reserving space in this task
   return false
end

function Task:_is_work_available()
   -- finally, if there's just nothing to do, there's just nothing to do.
   if self:_is_work_finished() then
      self._log:spam('work is finished, therefore no work is available.')
      return false
   end

   -- there's no work avaiable if we've already hit the cap of max workers allowed
   -- to start the task.
   local active_action_count = self:_get_active_action_count()
   if active_action_count >= self._max_workers then
      self._log:spam('active action count %d exceeds max workers %d', active_action_count, self._max_workers)
      return false
   end

   -- the amount of work left is just the number of times we're supposed to run
   -- minus the number of things actually done minus the number of people attempting
   -- to complete the task.  if that's 0, there's nothing left for anyone else.
   if not self._times then
      self._log:spam('task allowed to run an unlimited number of times')
      return true
   end

   local active_action_count = self:_get_active_action_count()
   local work_remaining = self._times - self._complete_count - active_action_count
   
   self._log:spam('work remaining: %d (times:%d, complete_count:%d, active_count:%d)', 
                  work_remaining, self._times, self._complete_count, active_action_count)

   return work_remaining > 0
end

function Task:_action_is_active(action)
   return self._running_actions[action]
end

function Task:_stop_feeding()
   if self._currently_feeding then
      self._log:detail('asking task_group to stop feeding this task')
      self._currently_feeding = false
      self._task_group:_stop_feeding_task(self)
   end
end

function Task:_start_feeding()
   if not self._currently_feeding then
      self._log:detail('asking task_group to start feeding this task')
      self._currently_feeding = true
      self._task_group:_start_feeding_task(self)
   end
end


function Task:_log_state(tag)
   if self._log:is_enabled(radiant.log.DETAIL) then
      local running_count = 0
      for _, _ in pairs(self._running_actions) do
         running_count = running_count + 1
      end
      local work_available = self:_is_work_available()
      self._log:detail('%s [state:%s running:%d completed:%d total:%s available:%s]',
                       tag, self._state, running_count, self._complete_count, self._times,
                       work_available)
   end
end

function Task:__action_try_start_thinking(action)
   self:_log_state('entering __action_try_start_thinking')

   -- a little bit of paranoia never hurt anyone.
   if self._state ~= STARTED then
      self._log:detail('task is not currently running.  cannot start_thinking! (state:%s)', self._state)
      return false;
   end

   local entity = action:get_entity()
   if self._workers_pending_unfeed[entity:get_id()] then
      self._log:detail('task worker %s pending to be removed.  cannot start_thinking!', entity)
      return false;
   end

   if not self:check_worker_against_task_affinity(entity) then
      return false
   end

   -- actions that are already running (or we've just told to run) are of course
   -- allowed to start.
   if self:_action_is_active(action) then
      self._log:detail('action is active.  can start!')
      return true
   end

   -- otherwise, only start when the task is active and there's stuff to do.
   local can_start = self._state == STARTED and self:_is_work_available()
   self._log:detail('can start? %s', tostring(can_start))
   return can_start
end

-- this protects races from many actions simultaneously trying to start at
-- the same time.
function Task:__action_try_start(action, worker)
   local entity = action:get_entity()

   self:_log_state(string.format('entering __action_try_start (entity:%s)', tostring(entity)))

   if self._state ~= STARTED then
      self._log:detail('task is not currently running.  cannot start! (state:%s)', self._state)
      return false
   end

   if self._workers_pending_unfeed[entity:get_id()] then
      self._log:detail('task worker %s is pending to be removed.  cannot start!', entity)
      return false
   end

   if not self:check_worker_against_task_affinity(worker) then
      return false
   end

   -- if the work "went away" before the call to __action_can_start and now, reject
   -- the request.  this usually happens because some other action ran in and grabbed
   -- the work item before you could.
   if not self:_is_work_available() then
      self:_log_state('cannot start.  no work available.')
      return false
   end
   
   self._running_actions[action] = true

   -- set the worker affinity timeout to something abnormally large.  this will make
   -- sure the worker remains bound to this task as we continue feeding it, no matter
   -- how long actually running this action takes.
   if self._affinity_timeout then
      self._worker_affinity_timeout[worker:get_id()] = INFINITE
   end

   -- if this is the last guy to get a job, signal everyone else to let them know
   -- that they don't have a shot (this may trigger them to stop_thinking)
   if not self:_is_work_available() then
      radiant.events.trigger_async(self, 'work_available', self, false)
      self:_stop_feeding()
   end
   if self._task_group then
      self._task_group:_notify_worker_started_task(self, action:get_entity())
   end

   self:_log_state('exiting __action_try_start')
   return true
end

function Task:__action_completed(action)
   self:_log_state('entering __action_completed')

   assert(self._running_actions[action])
   self._complete_count = self._complete_count + 1

   self:_log_state('exiting __action_completed')
end

function Task:__action_destroyed(action)
   self:_log_state('entering __action_destroyed')
   if self._task_group then
      -- assert(not self._running_actions[action])
   end
   self:_log_state('exiting __action_destroyed')
end

function Task:__action_stopped(action, worker)
   self:_log_state('entering __action_stopped')

   self._running_actions[action] = nil
   if self._affinity_timeout then
      -- reset the affinty timeout clock.  if the worker isn't fed before the
      -- timeout expires, it will lose it's affinity.
      self._worker_affinity_timeout[worker:get_id()] = stonehearth.calendar:get_elapsed_time() + self._affinity_timeout
   end

   if self._state ~= DESTROYED then
      if self:_is_work_available() then
         radiant.events.trigger_async(self, 'work_available', self, true)
         self:_start_feeding()
      end

      self._task_group:_notify_worker_stopped_task(self, action:get_entity())

      if self:_is_work_finished() then
         self._log:debug('task reached max number of completions (%d).  stopping and completing!', self._times)
         self:_set_state(COMPLETED)
         self:_destroy_entity_effects()
         self:_fire_cbs(COMPLETED, self._notify_completed_cbs)
         self._log:debug('destroying self after completion!')
         self:_destroy()
      end

   end
   
   self:_log_state('exiting __action_stopped')
end

return Task
