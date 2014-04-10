
local AIComponent = class()
local ExecutionFrame = require 'components.ai.execution_frame'
local log = radiant.log.create_logger('ai.component')

local action_key_to_activity = {}

function AIComponent:initialize(entity, json)
   self._entity = entity
   self._action_index = {}
   self._task_groups = {}
   self._observer_instances = {}
   self._sv = self.__saved_variables:get_data()
   self.__saved_variables:set_controller(self)

   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv._observer_datastores = {}
   end

   radiant.events.listen(entity, 'radiant:entity:post_create', function()
         self:_initialize(json)
         self:_start()
         return radiant.events.UNLISTEN
      end)
end

-- return a task group which instructs just this entity to perform
-- the specified action
function AIComponent:get_task_group(activity)
   local tg = self._task_groups[activity]
   if not tg then
      -- grab the scheduler for the town this entity is in, create
      -- a new task group which does this activity, and add our entity
      -- as the only worker.
      tg = stonehearth.town:get_town(self._entity)
                              :get_scheduler()
                                 :create_task_group(activity, {})
                                    :add_worker(self._entity)
      self._task_groups[activity] = tg
   end
   return tg
end

function AIComponent:get_entity()
   return self._entity
end

function AIComponent:get_debug_info()
   if self._execution_frame then
      return {
         execution_frame = self._execution_frame:get_debug_info(),
      } 
   end
   return {}
end

function AIComponent:destroy()
   self._dead = true
   self:_terminate_thread()

   for _, obs in pairs(self._observer_instances) do
      if obs.destroy then
         obs:destroy(self._entity)
      end
   end
   self._observer_instances = {}
   self._action_index = {}
end

function AIComponent:add_action(uri, injecting_entity)
   if injecting_entity == nil then
      self._sv.actions[uri] = true
      self.__saved_variables:mark_changed()
   end

   self:_add_action_script(uri, injecting_entity)
end

function AIComponent:_add_action_script(uri, injecting_entity)
   local ctor = radiant.mods.load_script(uri)
   self:_add_action(uri, ctor, injecting_entity)
end

function AIComponent:_add_action(key, action_ctor, injecting_entity)
   local does = action_ctor.does
   assert(does)
   assert(not action_key_to_activity[key] or action_key_to_activity[key] == does)
   
   action_key_to_activity[key] = does
   local action_index = self._action_index[does]
   if not action_index then
      action_index = {}
      self._action_index[does] = action_index
   end
   
   
   if self._action_index[does][key] then
      if self._action_index[does][key].action_ctor == action_ctor then
         log:debug('ignoring duplicate action in index (should we refcount?)')
         return
      end
      assert(false, string.format('duplicate action key "%s" for "%s"', tostring(key), tostring(does)))
   end
   
   local entry = {
      action_ctor = action_ctor,
      injecting_entity = injecting_entity,
   }
   self._action_index[does][key] = entry
   radiant.events.trigger(self, 'stonehearth:action_index_changed:' .. does, 'add', key, entry, does)
end

function AIComponent:remove_action(key)
   if type(key) == 'string' then
      self.__saved_variables:modify_data(function (o)
            o.actions[key] = false
         end)
   end

   local does = action_key_to_activity[key]
   if does then
      local entry = self._action_index[does][key]
      log:detail('triggering stonehearth:action_index_changed:' .. does)
      radiant.events.trigger(self, 'stonehearth:action_index_changed:' .. does, 'remove', key, entry, does)
      self._action_index[does][key] = nil
   else
      log:debug('could not find action for key %s in :remove_action', tostring(key))
   end
end

function AIComponent:add_custom_action(action_ctor, injecting_entity)
   log:debug('adding action "%s" (%s) to %s', action_ctor.name, tostring(action_ctor), self._entity)
   self:_add_action(action_ctor, action_ctor, injecting_entity)
end

function AIComponent:remove_custom_action(action_ctor, injecting_entity)
   log:debug('removing action "%s" (%s) from %s', action_ctor.name, tostring(action_ctor), self._entity)
   self:remove_action(action_ctor)
end

function AIComponent:add_observer(uri)
   self.__saved_variables:modify_data(function (o)
         o.observers[uri] = true
      end)
   self:_add_observer_script(uri)
end

function AIComponent:_add_observer_script(uri)
   local ctor = radiant.mods.load_script(uri)
   assert(not self._observer_instances[uri])

   local new_observer_instance = ctor(self._entity)
   
   --Do we have a datastore for this observer? if not, create one
   if not self._sv._observer_datastores[uri] then
      self._sv._observer_datastores[uri] = radiant.create_datastore()
      self.__saved_variables:mark_changed()
   end
   new_observer_instance.__saved_variables = self._sv._observer_datastores[uri]
   new_observer_instance:initialize(self._entity)

   self._observer_instances[uri] = new_observer_instance
end

function AIComponent:remove_observer(key)
   self._sv.observers = nil
   self.__saved_variables:mark_changed()

   local observer = self._observer_instances[key]
   if observer then
      if observer.destroy then
         observer:destroy()
      end
      self._observer_instances[key] = nil
   end
end


function AIComponent:_initialize(json)  
   if self._sv.actions then
      for uri, _ in pairs(self._sv.actions) do
         self:_add_action_script(uri)
      end
   else
      self._sv.actions = {}
      for _, uri in ipairs(json.actions or {}) do
         self:add_action(uri)
      end
   end
   
   if self._sv.observers then
      for uri, _ in pairs(self._sv.observers) do
         self:_add_observer_script(uri)
      end
   else
      self._sv.observers = {}
      for _, uri in ipairs(json.observers or {}) do
         self:add_observer(uri)
      end
   end
end

function AIComponent:_start()
   assert(not self._dead)
   assert(not self._thread)
   
   radiant.check.is_entity(self._entity)
   self._thread = stonehearth.threads:create_thread()
                                     :set_debug_name('e:%d', self._entity:get_id())

   self._thread:set_thread_main(function()
      self._execution_frame = self:_create_execution_frame()
      while not self._dead do
         self._execution_frame:run({})
         if self._execution_frame:get_state() == 'dead' then
            self._execution_frame = self:_create_execution_frame()
         else
            self._execution_frame:stop()
         end
      end
      self._execution_frame:destroy()
   end)
   self._thread:start()
end

function AIComponent:_create_execution_frame()
   local route = string.format('e:%d %s', self._entity:get_id(), radiant.entities.get_name(self._entity))
   self._thread:set_thread_data('stonehearth:run_stack', {})
   self._thread:set_thread_data('stonehearth:unwind_to_frame', nil)
   return ExecutionFrame(self._thread, route, self._entity, 'stonehearth:top', self._action_index)
end

function AIComponent:_terminate_thread()
   log:debug('terminating ai thread')
   if self._execution_frame then
      self._execution_frame:destroy('terminating thread')
   end
   if self._thread then
      local thread = self._thread
      self._thread = nil
      self._resume_error = nil
      
      -- If we're calling _terminate_thread() from the co-routine itself,
      -- this function MAY NOT RETURN!
      thread:terminate()
   end
end

function AIComponent:suspend_thread()
   assert(self._thread)
   stonehearth.threads:suspend_thread(self._thread)
   if self._resume_error then
      local msg = self._resume_error
      self._resume_error = nil
      error(msg)
   end
end

function AIComponent:resume_thread()
   assert(self._thread)
   stonehearth.threads:resume_thread(self._thread)
end

return AIComponent
