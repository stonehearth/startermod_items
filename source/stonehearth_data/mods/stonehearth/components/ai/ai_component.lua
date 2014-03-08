
local AIComponent = class()
local ExecutionFrame = require 'components.ai.execution_frame'
local log = radiant.log.create_logger('ai.component')

local action_key_to_activity = {}

function AIComponent:__init()
   self._observers = {}
   self._action_index = {}
   self._execution_count = 0
end

function AIComponent:__create(entity, json)
   self._entity = entity
   self.__savestate = radiant.create_datastore()
   self.__savestate:set_controller(self)
   radiant.events.listen(entity, 'stonehearth:entity:post_create', function()
         stonehearth.ai:start_ai(self._entity, self, json)
         return radiant.events.UNLISTEN
      end)
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

function AIComponent:__destroy()
   stonehearth.ai:stop_ai(self._entity:get_id(), self)

   self._dead = true
   self:_terminate_thread()

   for _, obs in pairs(self._observers) do
      if obs.destroy then
         obs:destroy(self._entity)
      end
   end
   self._observers = {}
   self._action_index = {}
end

function AIComponent:add_action(key, action_ctor, injecting_entity)
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
   radiant.events.trigger(self, 'stonehearth:action_index_changed', 'add', key, entry, does)
end

function AIComponent:remove_action(key)
   local does = action_key_to_activity[key]
   if does then
      local entry = self._action_index[does][key]
      self._action_index[does][key] = nil
      radiant.events.trigger(self, 'stonehearth:action_index_changed', 'remove', key, entry, does)
   end
end

function AIComponent:add_observer(key, observer)
   assert(not self._observers[key])
   self._observers[key] = observer
end

function AIComponent:remove_observer(observer)
   if observer then
      if observer.destroy_observer then
         observer:destroy_observer()
      end
      self._observer[key] = nil
   end
end

-- shouldn't need this...  threads should restart themselves... but
-- what if we die while trying to restart?  ug!
function AIComponent:restart_if_terminated()
   if not self._thread then
      self:restart()
   end
end

function AIComponent:restart()
   assert(not self._dead)

   radiant.check.is_entity(self._entity)
   self:_terminate_thread()
   
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
