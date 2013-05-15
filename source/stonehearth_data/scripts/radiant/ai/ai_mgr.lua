local om
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'
local BehaviorManager = require 'radiant.ai.behavior_manager'

local AIManager = class()

function AIManager:__init()
   self._action_registry = {}
   self._intention_registry = {}
   self._observer_registry = {}
   
   self._entities = {}
   self._behavior_managers = {}
   
   self._scheduled = {}
   self._co_to_bm = {}
   self._waiting_until = {}
   md:listen('radiant.events.gameloop', self)
end

AIManager['radiant.events.gameloop'] = function(self, now)
   -- xxx: this is O(n) of entities with ai. UG. make the intention notify the bm when its priorities change
   for id, bm in pairs(self._behavior_managers) do
      bm:check_intentions()
   end
   
   for co, pred in pairs(self._waiting_until) do
      if type(pred) == 'function' then
         if pred(now) then
            self:_schedule(co)
          end
      elseif type(pred) == 'number' then
         if env:now() >= pred then
            self:_schedule(co)
         end
      else
         assert(false)
      end
   end

   local dead = {}
   for co, _ in pairs(self._scheduled) do
      -- run it
      local status = coroutine.status(co)
      if status ~= 'suspended' then
         status = status
      end
      local success, wait_obj = coroutine.resume(co)
      if not success then
         check:report_thread_error(co, 'co-routine failed: ' .. tostring(wait_obj))
         self._scheduled[co] = nil
      end
     
      local status = coroutine.status(co)
      if status == 'suspended' and wait_obj then
         self._scheduled[co] = nil
         self._waiting_until[co] = wait_obj
      else
         table.insert(dead, co)
      end
   end
   for _, co in ipairs(dead) do
      local bm = self._co_to_bm[co]
      if bm then
         bm:restart()
      end
   end
end

function AIManager:register_action(name, ctor)
   check:verify(self._action_registry[name] == nil)
   check:verify(ctor ~= nil)
   
   self._action_registry[name] = ctor
end

function AIManager:register_intention(name, ctor)
   check:verify(self._intention_registry[name] == nil)
   self._intention_registry[name] = ctor
end

function AIManager:register_observer(name, ctor)
   check:verify(self._observer_registry[name] == nil)
   self._observer_registry[name] = ctor
end

function AIManager:add_intention(entity, name, ...)
   local ctor = self._intention_registry[name]
   assert(ctor)
   local bm = self:_get_bm(entity)
   return bm:add_intention(name, ctor(entity, ...))
end

function AIManager:add_observer(entity, name, ...)
   local ctor = self._observer_registry[name]
   assert(ctor)
   local bm = self:_get_bm(entity)
   return bm:add_observer(ctor(entity, ...))  
end

function AIManager:add_debug_hook(entity_id, hookfn)
   local bm = self._behavior_managers[entity_id]
   if bm then
      bm:set_debug_hook(hookfn)
      return true
   end
end

function AIManager:_get_bm(arg0)
   local id
   local entity
   if type(arg0) == 'number' then
      id = arg0
      entity = self._entities[id]
   else
      entity = arg0
      id = entity:get_id()
   end
   check:is_entity(entity)
   
   if not self._behavior_managers[id] then
      if not om then
         om = require 'radiant.core.om'   
      end
      
      self._behavior_managers[id] = BehaviorManager(entity)
      om:on_destroy_entity(entity, function()
         self._behavior_managers[id]:destroy()
         self._behavior_managers[id] = nil
      end)
   end
   
   return self._behavior_managers[id]
end

function AIManager:init_entity(entity, ai)
   local id = entity:get_id()
   
   self._entities[id] = entity  
   if ai.intentions then
      for _, name in ipairs(ai.intentions) do
         self:add_intention(entity, name)
      end
   end
  
   if ai.observers then
      for _, name in ipairs(ai.observers) do
         self:add_observer(entity, name)
      end
   end
end

function AIManager:_create_action(entity, activity)
   local name = select(1, unpack(activity))

   check:verify(self._action_registry[name])
   local ctor = self._action_registry[name]
   return ctor(entity)
end

function AIManager:_create_thread(bm, fn)
   local co = coroutine.create(fn)
   self._scheduled[co] = true
   self._co_to_bm[co] = bm
   return co
end

function AIManager:_terminate_thread(co)
   if co then
      self._waiting_until[co] = nil
      self._scheduled[co] = nil
      self._co_to_bm[co] = nil
   end
end

function AIManager:_wait_until(co, cb)
   assert(self._scheduled[co] ~= nil)
   self._waiting_until[co] = cb
end

function AIManager:_schedule(co)
   self._scheduled[co] = true
   self._waiting_until[co] = nil
end

return AIManager()
