
local AIComponent = class()
local ExecutionFrame = require 'components.ai.execution_frame'

local action_key_to_activity = {}

function AIComponent:initialize(entity, json)
   self._entity = entity
   self._action_index = {}
   self._task_groups = {}
   self._observer_instances = {}
   self._all_execution_frames = {}
   self._sv = self.__saved_variables:get_data()
   self.__saved_variables:set_controller(self)
   self._aitrace = radiant.log.create_logger('ai_trace')
   self._aitrace:set_prefix('e' .. tostring(entity:get_id()) .. '/')
   local s = radiant.entities.get_name(entity) or 'noname'
   self._aitrace:spam('@ce@%s@%s', 'e' .. tostring(entity:get_id()), s)

   self._log = radiant.log.create_logger('ai.component')
                          :set_entity(self._entity)

   if not self._sv._initialized then
      -- wait until the entity is completely initialized before piling all our
      -- observers and actions
      radiant.events.listen_once(entity, 'radiant:entity:post_create', function()
            self:_initialize(json)
         end)

      -- wait until the very next gameloop to start our thread.  this gives the
      -- person creating the entity time to do some more post-creating initialization
      -- (e.g. setting the player id!).  xxx: it's probably better to do this by
      -- passing an init function to create_entity(). -tony
      radiant.events.listen_once(radiant, 'stonehearth:gameloop', function()
            if not self._dead then
               self:_start()
            end
         end)
   else
      --we're loading so instead listen on game loaded
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            self:_initialize(json)
            self:_start()
         end)
   end
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

function AIComponent:set_status_text(text)
   self._sv.status_text = text
   self.__saved_variables:mark_changed()
end

function AIComponent:add_action(uri, injecting_entity)
   if injecting_entity == nil then
      self._sv._actions[uri] = true
      self.__saved_variables:mark_changed()
   end

   self:_add_action_script(uri, injecting_entity)
end

function AIComponent:_add_action_script(uri, injecting_entity)
   local ctor = radiant.mods.load_script(uri)
   self:_add_action(uri, ctor, injecting_entity)
end

function AIComponent:_action_key_to_name(key)
   if type(key) == 'table' then
      return key.name
   else
      return key
   end
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
         self._log:debug('ignoring duplicate action in index (should we refcount?)')
         return
      end
      assert(false, string.format('duplicate action key "%s" for "%s"', tostring(key), tostring(does)))
   end

   --self._log:spam('%s, ai_component:add_action: %s', self._entity, self:_action_key_to_name(key))

   local entry = {
      action_ctor = action_ctor,
      injecting_entity = injecting_entity,
   }
   self._action_index[does][key] = entry
   self:_notify_action_index_changed(does, 'add', key, entry)
end

function AIComponent:_notify_action_index_changed(activity_name, add_remove, key, entry)
   local frames = self._all_execution_frames[activity_name]
   if frames then
      self._notifying_action_index_changed = true
      for frame, _ in pairs(frames) do
         frame:on_action_index_changed(add_remove, key, entry)
      end
      self._notifying_action_index_changed = false
   end
end

function AIComponent:create_execution_frame(activity_name, debug_route, trace_route)
   local frame = ExecutionFrame(self._thread, self._entity, self._action_index, activity_name, debug_route, trace_route)
   local frames = self._all_execution_frames[activity_name]
   self:_register_execution_frame(activity_name, frame)
   return frame
end

function AIComponent:_register_execution_frame(activity_name, frame)
   local frames = self._all_execution_frames[activity_name]
   if not frames then
      frames = {}
      self._all_execution_frames[activity_name] = frames
   end
   frames[frame] = true
end

function AIComponent:_unregister_execution_frame(activity_name, frame)
   local frames = self._all_execution_frames[activity_name]
   if frames then
      frames[frame] = nil
   end
end

function AIComponent:remove_action(key)
   if type(key) == 'string' then
      self.__saved_variables:modify_data(function (o)
            o._actions[key] = nil
         end)
   end

   local does = action_key_to_activity[key]
   if does then
      local entry = self._action_index[does][key]
      --self._log:spam('%s, ai_component:remove_action: %s', self._entity, self:_action_key_to_name(key))
      self._log:detail('triggering stonehearth:action_index_changed:' .. does)
      self._action_index[does][key] = nil
      self:_notify_action_index_changed(does, 'remove', key, entry)
   else
      self._log:debug('could not find action for key %s in :remove_action', tostring(key))
   end
end

function AIComponent:add_custom_action(action_ctor, injecting_entity)
   self._log:debug('adding action "%s" (%s) to %s', action_ctor.name, tostring(action_ctor), self._entity)
   self:_add_action(action_ctor, action_ctor, injecting_entity)
end

function AIComponent:remove_custom_action(action_ctor, injecting_entity)
   self._log:debug('removing action "%s" (%s) from %s', action_ctor.name, tostring(action_ctor), self._entity)
   self:remove_action(action_ctor)
end

function AIComponent:add_observer(uri)
   if self._observer_instances[uri] then
      return
   end

   local ctor = radiant.mods.load_script(uri)   
   local observer = ctor(self._entity)
   
   if not self._sv._observer_datastores[uri] then
      self._sv._observer_datastores[uri] = radiant.create_datastore()
   end

   observer.__saved_variables = self._sv._observer_datastores[uri]
   observer:initialize(self._entity)

   self._observer_instances[uri] = observer
   self.__saved_variables:mark_changed()
end

function AIComponent:remove_observer(uri)
   local observer = self._observer_instances[uri]

   if observer then
      if observer.destroy then
         observer:destroy()
      end

      self._observer_instances[uri] = nil
      self._sv._observer_datastores[uri] = nil
      self.__saved_variables:mark_changed()
   end
end

function AIComponent:_initialize(json)
   if not self._sv._initialized then
      self._sv.status_text = ''
      self._sv._actions = {}
      self._sv._observer_instances = {}
      self._sv._observer_datastores = {}

      for _, uri in ipairs(json.actions or {}) do
         self:add_action(uri)
      end

      for _, uri in ipairs(json.observers or {}) do
         self:add_observer(uri)
      end
   else
      for uri, _ in pairs(self._sv._actions) do
         self:_add_action_script(uri)
      end

      for uri, _ in pairs(self._sv._observer_datastores) do
         self:add_observer(uri)
      end
   end

   self._sv._initialized = true
   self.__saved_variables:mark_changed()
end

function AIComponent:_start()
   assert(not self._dead)
   assert(not self._thread)
   
   radiant.check.is_entity(self._entity)
   self._thread = stonehearth.threads:create_thread()
                                     :set_debug_name('e:%d', self._entity:get_id())

   self._sv.status_text = ''
   self.__saved_variables:mark_changed()

   self._thread:set_thread_main(function()
      self._execution_frame = self:_create_top_execution_frame()
      while not self._dead do
         local start_tick = radiant.gamestate.now()
         self._aitrace:spam('@loop')
         self._log:debug('starting new execution frame run in ai loop')
         self._execution_frame:run({})
         self._log:debug('reached bottom of execution frame run in ai loop')
         radiant.events.trigger(self._entity, 'stonehearth:ai:halt')
         
         -- Don't go back to running if we thought without ever yielding (i.e. without doing 
         -- any real work).  This also prevents tight ai loops that never yield (possibly 
         -- because some unit keeps aborting).
         while radiant.gamestate.now() - start_tick == 0 do
            self._log:info('yielding thread because we thought without yielding.')
            self._thread:sleep_realtime(0)
         end

         if self._execution_frame:get_state() == 'dead' then
            self._execution_frame = self:_create_top_execution_frame()
         else
            self._execution_frame:stop()
         end
      end
      if self._execution_frame then
         self._execution_frame:destroy()
         self._execution_frame = nil
      end
   end)
   self._thread:start()
end

function AIComponent:_create_top_execution_frame()
   self._thread:set_thread_data('stonehearth:run_stack', {})
   self._thread:set_thread_data('stonehearth:unwind_to_frame', nil)
   local traceroute = string.format('%s/', 'e' .. tostring(self._entity:get_id()))
   return self:create_execution_frame('stonehearth:top', '', traceroute)
end

function AIComponent:_terminate_thread()
   self._log:debug('terminating ai thread')
   if self._execution_frame then
      self._execution_frame:destroy('terminating thread')
      self._execution_frame = nil
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
