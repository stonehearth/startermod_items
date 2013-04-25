local BehaviorManager = class()

local log = require 'radiant.core.log'
local md = require 'radiant.core.md'
local check = require 'radiant.core.check'
local Destroyer = require 'radiant.core.destroyer'
local ai_mgr

function BehaviorManager:__init(entity)
   if not ai_mgr then
      ai_mgr = require 'radiant.ai.ai_mgr'
   end
   assert(entity)
   self._entity = entity
   self._observers = {}
   self._intentions = {}
   self._action_stack = {}
end

function BehaviorManager:destroy()
   self._dead = true
   self:_clear_action_stack()
   ai_mgr:_terminate_thread(self._co)
   if self._debug_hook then
      self._debug_hook:notify_current_thread(nil)
   end
   
   for obs, _ in pairs(self._observers) do
      if obs.destroy then
         obs:destroy(self._entity)
      end
   end
   for name, intention in pairs(self._intentions) do
      if intention.destroy then
         intention:destroy(self._entity)
      end
   end
   self._observers = {}
   self._intentions = {}
end

function BehaviorManager:set_debug_hook(hookfn)
   self._debug_hook = hookfn
   self._debug_hook:notify_current_thread(self._co)
end

function BehaviorManager:get_debug_hook()
   return self._debug_hook
end

function BehaviorManager:add_intention(name, intention)
   assert(not self._intentions[name])
   self._intentions[name] = intention
   return Destroyer(function()
         if intention.destroy then
            intention:destroy()
         end
         self._intentions[name] = nil
      end)
end

function BehaviorManager:add_observer(observer)
   self._observers[observer] = true
   return Destroyer(function()
         if observer.destroy then
            observer:destroy()
         end
         self._observers[observer] = nil
      end)
end

function BehaviorManager:check_intentions()
   check:is_entity(self._entity)
   
   local activity = self:_get_best_activity()
   if not self._current_activity or not self:_activities_equal(activity, self._current_activity) then     
      log:debug('behavior switching entity %s behavior:', tostring(self._entity))
      log:debug('   from: %s', self:_format_activity(self._current_activity))
      log:debug('   to:   %s', self:_format_activity(activity))
      self:restart(activity)
   end
end

function BehaviorManager:restart(activity)
   assert(not self._dead)
   
   check:is_entity(self._entity)
   self:_clear_action_stack()
   
   self._current_activity = activity and activity or self:_get_best_activity()
   assert(self._current_activity)
      
   log:debug('restarting behavior manager coroutine for: %s.', self:_format_activity(self._current_activity))
   ai_mgr:_terminate_thread(self._co)
   self._co = ai_mgr:_create_thread(self, function()
      while not self._dead do
         assert(self._current_activity)
         local running_activity = self._current_activity -- xxx: can we call _get_best_activity as first thing in the loop?
         log:debug('coroutine starting activity: %s', self:_format_activity(running_activity))        
         self:_execute_activity(self._current_activity)
         log:debug('coroutine finished: %s', self:_format_activity(running_activity))

         if not self._dead then         
            self._current_activity = self:_get_best_activity()
            log:debug('coroutine selected new activity: %s', self:_format_activity(self._current_activity))
         end
      end
   end)
   if self._debug_hook then
      self._debug_hook:notify_current_thread(self._co)
   end
end

function BehaviorManager:_get_best_activity()
   check:is_entity(self._entity)   
   local debug_info = { priorities = {} }
   
   log:debug('computing best activity for %s.', tostring(self._entity))
   local bp, ba = -1, nil
   for name, intention in pairs(self._intentions) do      
      local p, a = intention:recommend_activity(self._entity)
      if p ~= nil then
         log:debug('  recommend activity %11d %s', p, self:_format_activity(a))
         if self._debug_hook then
            debug_info.priorities[name] = {  priority = p, activity = self:_format_debug_activity(a) }
         end
         if p > bp then
            bp, ba = p, a
         end      
      end
   end
   if self._debug_hook then
      self._debug_hook:set_ai_priorities(debug_info)
   end
   assert(ba)
   log:debug('best activity for %s is %s.', tostring(self._entity), self:_format_activity(ba))
   return ba
end

function BehaviorManager:_activities_equal(a, b)
   if not a or not b then
      return false
   end

   assert(type(a) == 'table')
   assert(type(b) == 'table')
   if #a ~= #b then
      return false
   end
   for i = 1, #a do
      local a_i = a[i]
      local b_i = b[i]
      
      if a_i == b_i then
         return true
      end
      
      local a_type = type(a_i)
      local b_type = type(b_i)
      if a_type ~= b_type then
         return false
      end
      
      if a_type == 'userdata' then
         local a_key = a_i.get_id and a_i:get_id() and a_i:__towatch()
         local b_key = b_i.get_id and b_i:get_id() and b_i:__towatch()
         return a_key == b_key
      end
      
      -- table?  etc...
      return false
   end
   return true
end

function BehaviorManager:_format_activity(activity)
   if not activity then
      return 'none'
   end
   local msg = activity[1]
   for i = 2,#activity do
      msg = msg .. ", " .. tostring(activity[i])
   end
   return "{" .. msg .."}"
end

function BehaviorManager:_format_debug_activity(activity)
   local result = {}   
   for i = 1,#activity do
      local entry = activity[i]
      local et = type(entry)
      if et == 'number' or et == 'string' then
         result[i] = entry
      else 
         result[i] = tostring(entry)
      end
   end
   return result
end

function BehaviorManager:_clear_action_stack()
   for i = #self._action_stack, 1, -1 do
      local action = self._action_stack[i]
      if action.stop then
         action:stop(self, self._entity, false)
      end
   end
   self._action_stack = {}
   self._current_activity = nil
end

function BehaviorManager:abort(reason)
   -- xxx: assert that we're running inthe context of the coroutine
   assert(false)
end

function BehaviorManager:execute(...)
   return self:_execute_activity({...})
end

function BehaviorManager:_execute_activity(activity)
   check:is_entity(self._entity)
   
   local action = ai_mgr:_create_action(self._entity, activity)

   table.insert(self._action_stack, action)
   local len = #self._action_stack

   local result = { action:run(self, self._entity, select(2, unpack(activity))) }
   if self._dead then
      assert(#self._action_stack == 0)
      return
   end
   
   if action.stop then
      action:stop(self, self._entity, true)
   end
   
   assert(#self._action_stack == len)
   assert(self._action_stack[len] == action)
   self._action_stack[len] = nil
   return unpack(result)
end

function BehaviorManager:wait_until(obj)
   coroutine.yield(obj)
end

return BehaviorManager
