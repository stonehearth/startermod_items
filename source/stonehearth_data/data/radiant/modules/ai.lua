local ai = {}
local singleton = {}
local BehaviorManager = require 'radiant.modules.ai.behavior_manager'

function ai.__init()
   ai.SUSPEND_THREAD = {}
   singleton._action_registry = {}
   singleton._intention_registry = {}
   singleton._observer_registry = {}
   singleton._entities = {}
   singleton._behavior_managers = {}
   singleton._scheduled = {}
   singleton._co_to_bm = {}
   singleton._waiting_until = {}
end

function ai._on_event_loop(_, now)
   -- xxx: this is O(n) of entities with singleton. UG. make the intention notify the bm when its priorities change
   for id, bm in pairs(singleton._behavior_managers) do
      bm:check_action_stack()
   end
   
   for co, pred in pairs(singleton._waiting_until) do
      local run = false
      if type(pred) == 'function' then
         run = pred(now)
      elseif type(pred) == 'number' then
         run = env:now() >= pred
      elseif pred == ai.SUSPEND_THREAD then
         run = false
      else
         assert(false)
      end
      if run then
         ai._schedule(co)
      end
   end

   local dead = {}
   for co, _ in pairs(singleton._scheduled) do
      -- run it
      local status = coroutine.status(co)
      if status ~= 'suspended' then
         status = status
      end
      local success, wait_obj = coroutine.resume(co)
      if not success then
         radiant.check.report_thread_error(co, 'co-routine failed: ' .. tostring(wait_obj))
         singleton._scheduled[co] = nil
      end
     
      local status = coroutine.status(co)
      if status == 'suspended' and wait_obj then
         singleton._scheduled[co] = nil
         singleton._waiting_until[co] = wait_obj
      else
         table.insert(dead, co)
      end
   end
   for _, co in ipairs(dead) do
      local bm = singleton._co_to_bm[co]
      if bm then
         bm:restart()
      end
   end
end

function ai.add_action(entity, uri, ...)
   local ctor = radiant.mods.require(uri)  
   local bm = ai._get_bm(entity)
   bm:add_action(uri, ctor(bm, entity))
end

function ai.add_observer(entity, name, ...)
   assert(false)
   local ctor = singleton._observer_registry[name]
   assert(ctor)
   local bm = singleton._get_bm(entity)
   return bm:add_observer(ctor(entity, ...))  
end

function ai.add_debug_hook(entity_id, hookfn)
   assert(false)  
   local bm = singleton._behavior_managers[entity_id]
   if bm then
      bm:set_debug_hook(hookfn)
      return true
   end
end

function ai._get_bm(arg0)
   local id
   local entity
   if type(arg0) == 'number' then
      id = arg0
      entity = singleton._entities[id]
   else
      entity = arg0
      id = entity:get_id()
   end
   
   if not singleton._behavior_managers[id] then
      singleton._behavior_managers[id] = BehaviorManager(entity)
      radiant.entities.on_destroy(entity, function()
         singleton._behavior_managers[id]:destroy()
         singleton._behavior_managers[id] = nil
      end)
   end
   
   return singleton._behavior_managers[id]
end

function ai._init_entity(entity, obj)
   local id = entity:get_id()
   singleton._entities[id] = entity  
   if obj.actions then
      for _, uri in radiant.resources.pairs(obj.actions) do
         ai.add_action(entity, uri)
      end
   end
  
   if obj.observers then
      for _, uri in radiant.resources.pairs(obj.observers) do
         ai.add_observer(entity, uri)
      end
   end
end

function ai._create_action(entity, activity)
   local name = select(1, unpack(activity))

   radiant.check.verify(singleton._action_registry[name])
   local ctor = singleton._action_registry[name]
   return ctor(entity)
end

function ai._create_thread(bm, fn)
   local co = coroutine.create(fn)
   singleton._scheduled[co] = true
   singleton._co_to_bm[co] = bm
   return co
end

function ai._resume_thread(co)
   local wait_obj = singleton._waiting_until[co]
   if wait_obj == ai.SUSPEND_THREAD then
      ai._schedule(co)
   end
end

function ai._terminate_thread(co)
   if co then
      singleton._waiting_until[co] = nil
      singleton._scheduled[co] = nil
      singleton._co_to_bm[co] = nil
   end
end

function ai._wait_until(co, cb)
   assert(self._scheduled[co] ~= nil)
   self._waiting_until[co] = cb
end

function ai._schedule(co)
   singleton._scheduled[co] = true
   singleton._waiting_until[co] = nil
end

ai.__init()
radiant.events.listen('radiant.events.gameloop', ai._on_event_loop)

return ai
