local AiService = class()
local AiInjector = require 'services.ai.ai_injector'
local ExecutionUnitV1 = require 'components.ai.execution_unit_v1'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local CompoundActionFactory = require 'services.ai.compound_action_factory'
local placeholders = require 'services.ai.placeholders'
local log = radiant.log.create_logger('ai.service')

function AiService:__init()
   -- SUSPEND_THREAD is a unique, non-integer token which indicates the thread
   -- should suspend.  It must be non-intenger, as yielding an int means "wait
   -- until this time".  By creating a table, we guarantee the value of
   -- SUSPEND_THREAD is unique when compared with ==.  The name is for
   -- cosmetic purposes and aids in debugging.
   self.SUSPEND_THREAD = { name = "SUSPEND_THREAD" }
   self.KILL_THREAD = { name = "KILL_THREAD" }

   self._action_registry = {}
   self._observer_registry = {}
   self._entities = {}
   self._ai_components = {}
   self._scheduled = {}
   self._co_to_ai_component = {}
   self._waiting_until = {}
   
   for name, value in pairs(placeholders) do
      AiService[name] = value
   end

   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._on_event_loop)
end

function AiService:_on_event_loop(e)
   local now = e.now
   -- xxx: this is O(n) of entities with self. UG. make the intention notify the ai_component when its priorities change
   for id, ai_component in pairs(self._ai_components) do
      ai_component:restart_if_terminated()
   end

   for co, pred in pairs(self._waiting_until) do
      local run = false
      if type(pred) == 'function' then
         run = pred(now)
      elseif type(pred) == 'number' then
         run = radiant.gamestate.now() >= pred
      elseif pred == self.SUSPEND_THREAD then
         run = false
      else
         assert(false)
      end
      if run then
         self:_schedule(co)
      end
   end

   local dead = {}
   for co, _ in pairs(self._scheduled) do
      -- run it
      self._running_thread = co
      local success, wait_obj = coroutine.resume(co)
      self._running_thread = nil
      
      if not success then
         radiant.check.report_thread_error(co, 'entity ai error: ' .. tostring(wait_obj))
         self._scheduled[co] = nil
      end

      local status = coroutine.status(co)
      if status == 'suspended' and wait_obj and wait_obj ~= self.KILL_THREAD then
         self._scheduled[co] = nil
         self._waiting_until[co] = wait_obj
      else
         table.insert(dead, co)
      end
   end
  
   for _, co in ipairs(dead) do
      local ai_component = self._co_to_ai_component[co]
      if ai_component then
         ai_component:restart()
      end
   end
end

-- injecting entity may be null
function AiService:inject_ai(entity, injecting_entity, ai) 
   return AiInjector(entity, injecting_entity, ai)
end

-- injecting entity may be null
function AiService:add_action(entity, uri, injecting_entity)
   local ctor = radiant.mods.load_script(uri)
   local ai_component = self:_get_ai_component(entity)

   ai_component:add_action(uri, ctor, injecting_entity)
end

function AiService:remove_action(entity, uri)
   local ai_component = self:_get_ai_component(entity)
   ai_component:remove_action(uri)
end

function AiService:add_custom_action(entity, action_ctor, injecting_entity)
   local ai_component = self:_get_ai_component(entity)
   ai_component:add_action(action_ctor, action_ctor, injecting_entity)
end

function AiService:remove_custom_action(entity, action_ctor, injecting_entity)
   local ai_component = self:_get_ai_component(entity)
   ai_component:remove_action(action_ctor)
end

function AiService:add_observer(entity, uri, ...)
   local ctor = radiant.mods.load_script(uri)
   local ai_component = self:_get_ai_component(entity)
   ai_component:add_observer(uri, ctor(entity, ...))
end

function AiService:remove_observer(entity, observer)
   local ai_component = self:_get_ai_component(entity)
   ai_component:remove_observer(observer)
end

function AiService:_get_ai_component(arg0)
   local id, entity
   if type(arg0) == 'number' then
      id = arg0
      entity = self._entities[id]
   else
      entity = arg0
      id = entity:get_id()
   end
   local ai_component = self._ai_components[id]
   assert(ai_component)
   return ai_component
end

function AiService:start_ai(entity, obj)
   local id = entity:get_id()
   local ai_component = entity:get_component('stonehearth:ai')
   assert(ai_component)

   self._entities[id] = entity
   self._ai_components[id] = ai_component
   if obj.actions then
      for _, uri in ipairs(obj.actions) do
         self:add_action(entity, uri)
      end
   end

   if obj.observers then
      for _, uri in ipairs(obj.observers) do
         self:add_observer(entity, uri)
      end
   end
end

function AiService:stop_ai(id)
   self._entities[id] = nil
   self._ai_components[id] = nil
end

function AiService:_create_thread(ai_component, fn)
   local co = coroutine.create(fn)
   self._scheduled[co] = true
   self._co_to_ai_component[co] = ai_component
   return co
end

function AiService:_resume_thread(co)
   local wait_obj = self._waiting_until[co]
   if wait_obj == self.SUSPEND_THREAD then
      self:_schedule(co)
   end
end

-- removes the thread from the scheduler and the waiting thread
-- list.  If the thread is still running (terminate self?  is that
-- moral?), you still need to _complete_thread_termination later,
-- which will make sure the thread doesn't get rescheduled.
function AiService:_terminate_thread(co)
   if co then
      self._waiting_until[co] = nil
      self._scheduled[co] = nil
      self._co_to_ai_component[co] = nil
      
      if self._running_thread ~= co then
         log:info('killing non running thread... nothing to do.')
      else 
         log:info('killing running thread... yielding KILL_THREAD.')
         coroutine.yield(self.KILL_THREAD)
      end
   end
end

function AiService:_wait_until(co, cb)
   assert(self._scheduled[co] ~= nil)
   self._waiting_until[co] = cb
end

function AiService:_schedule(co)
   self._scheduled[co] = true
   self._waiting_until[co] = nil
end

function AiService:format_activity(activity)
   return activity.name .. '(' .. self:format_args(activity.args) .. ')'
end

function AiService:format_args(args)
   local msg = ''
   if args then
      for name, value in pairs(args) do
         if #msg > 0 then
            msg = msg .. ', '
         end
         msg = msg .. string.format('%s = %s ', name, radiant.util.tostring(value))
      end
   end
   return '{ ' .. msg .. '}'
end

function AiService:create_execution_unit(ai_component, debug_route, action_ctor, entity, injecting_entity)
   local execution_unit_ctor
   if not action_ctor.version or action_ctor.version == 1 then
      execution_unit_ctor = ExecutionUnitV1
   else
      execution_unit_ctor = ExecutionUnitV2
   end
   local unit = execution_unit_ctor(ai_component, entity, injecting_entity)
   local ai_interface = unit:get_action_interface()

   local action
   if action_ctor.create_action then
      action = action_ctor:create_action(ai_interface, entity, injecting_entity)
   else
      action = action_ctor(ai_interface, entity, injecting_entity)
   end
   unit:set_action(action)
   if debug_route then
      unit:set_debug_route(debug_route)
   end
   return unit
end

function AiService:create_compound_action(action_ctor)
   assert(action_ctor)
   -- cannot implement anything here.  it gets really confusing (where does start_thinking forward
   -- its args to?  the next action in the chain or the calling action?)
   assert(not action_ctor.start_thinking, 'compound actions must not contain implementation')
   assert(not action_ctor.stop_thinking, 'compound actions must not contain implementation')
   
   return CompoundActionFactory(action_ctor)
end

function AiService:create_activity(name, args)   
   if args == nil then
      args = {}
   end
   assert(type(name) == 'string', 'activity name must be a string')
   assert(type(args) == 'table', 'activity arguments must be an associative array')
   assert(not args[1], 'activity arguments contains numeric elements (invalid!)')

   -- common error... trying to pass a class instance (e.g. 'foo', { my_instance })
   -- this won't catch them all, but will catch all uses of unclasslib clases
   assert(not args.__class, 'attempt to pass class for activity args (not using associative array?)')
   assert(not args.__type,  'attempt to pass instance for activity args (not using associative array?)')

   return {
      name = name,
      args = args
   }
end


return AiService()
