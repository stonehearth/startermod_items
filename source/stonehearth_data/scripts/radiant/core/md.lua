local log = require 'radiant.core.log'
local check = require 'radiant.core.check'
local util = require 'radiant.core.util'

local MsgDispatcher = class()

function MsgDispatcher:__init()
   self._now = -1;

   self._registered_activities = {}    -- all possible activities, by name
   self._registered_handlers = {}      -- all registered msg handlers, by name
   self._registered_msgs = {}          -- all registered msgs, by name
   self._registered_msg_filters = {}   -- filters for all registered msgs, by name

   self._all_handlers = {}             -- translates handlers to their names
   self._entity_msg_handlers = {}      -- all msg handlers for all entities
   
   self._entity_listen_map = {}          -- listen map for handlers, by id
   self._free_handlers   = {}          -- all handlers who have called listen without an entity
   
   self._msg_queue = {}                -- list of messages yet to be sent.
   
   self:register_msg('radiant.md.create')
   self:register_msg('radiant.md.destroy')
   self:register_msg('radiant.events.gameloop')
   self:register_msg('radiant.commands.activate')
   
   self:register_msg('radiant.events.slow_poll')   
   self:register_msg('radiant.events.very_slow_poll')
   
   self:register_msg('radiant.events.aura_expired')
end

function MsgDispatcher:update(now)
   self._now = now
   self:broadcast_msg('radiant.events.gameloop', self._now)
   -- pump the polls
   if now % 200 == 0 then
      self:broadcast_msg('radiant.events.slow_poll', self._now)
   end  
   if now % 1000 == 0 then   
      self:broadcast_msg('radiant.events.very_slow_poll', self._now)
   end
   self:_flush_msg_queue();
end

function MsgDispatcher:now()
   return self._now
end

function MsgDispatcher:is_msg(msg)
   return type(msg) == 'string' and self._registered_msgs[msg] ~= nil
end

function MsgDispatcher:is_msg_filter(filter)
   return type(filter) == 'string' and self._registered_msg_filters[filter] ~= nil
end

function MsgDispatcher:is_msg_handler(handler)
   return self._all_handlers[handler] ~= nil
end

-- xxx: make this a Lua Iterator (or better yet, cache em!)
function MsgDispatcher:_get_msg_filters(filter)
   filters = {}
   table.insert(filters, filter)
   repeat
      filter = filter:gsub('(.*)%.(.*)', '%1') -- the lua regexp escape char is %.  nice...
      table.insert(filters, filter)
   until not filter:find('%.')
   return filters
end

function MsgDispatcher:register_msg(msg)
   log:info('registering msg "%s".', msg)
   check:verify(not self:is_msg_filter(msg))
   check:verify(not env:is_running())

   self._registered_msgs[msg] = true
   for _, filter in ipairs(self:_get_msg_filters(msg)) do
      self._registered_msg_filters[filter] = true
   end
end

function MsgDispatcher:register_msg_handler(name, handler)
   check:is_string(name)
   assert(not env:is_running()) 
   assert(not self._registered_handlers[name])
   check:is_callable(handler)
   
   log:info('registering msg_handler "%s".', name)
   self._registered_handlers[name] = handler
end

-- xxx: dangerous... how do these get loaded and saved?
function MsgDispatcher:register_msg_handler_instance(name, handler)
   check:is_string(name)
   assert(not self._registered_handlers[name])
   assert(not self:is_msg_handler(handler))
   self._all_handlers[handler] = name   
end

-- xxx: this belongs in the BehaviorManager, not the MsgDispatcher
function MsgDispatcher:register_activity(...)
   local name = select(1, ...)
   check:is_string(name)
   check:verify(not self:is_activity_name(name))

   log:debug('registering activity "%s".', name)
   self._registered_activities[name] = ...
end

-- xxx: this belongs in the BehaviorManager, not the MsgDispatcher
function MsgDispatcher:is_activity(a)
   return type(a) == 'table' and self:is_activity_name(a.name)
end

-- xxx: this belongs in the BehaviorManager, not the MsgDispatcher
function MsgDispatcher:is_activity_name(name)
   return type(name) == 'string' and self._registered_activities[name]
end


-- xxx: this belongs in the BehaviorManager, not the MsgDispatcher
function MsgDispatcher:create_activity(...)
   local activity_name = select(1, ...)
   check:verify(self:is_activity_name(activity_name))

   log:debug('creating activity "%s".', activity_name)
   local result = {
      name = activity_name,
      args = {select(2, ...)}
   }
   return result;
end

function MsgDispatcher:create_msg_handler(name, ...)
   assert(env:is_running()) 
   assert(self._registered_handlers[name])
   assert(name)
   
   local ctor = self._registered_handlers[name]
   assert(ctor)

   local handler = ctor();
   handler = handler and handler or ctor
   self._all_handlers[handler] = name   
   self:call_handler(handler, 'radiant.md.create', ...)

   return handler
end

function MsgDispatcher:add_msg_handler(entity, name, ...)
   check:is_entity(entity)
   check:verify(self._registered_handlers[name])
   
   local handler = self:create_msg_handler(name, entity, ...) 

   -- track which entity the handler is attached to so we can implement
   -- send_msg, save, load, etc.
   local id = entity:get_id();
   if not self._entity_msg_handlers[id] then
      self._entity_msg_handlers[id] = {}
   end  
   self._entity_msg_handlers[id][handler] = true
     
   return handler
end

function MsgDispatcher:remove_msg_handler(entity, name)
   check:is_entity(entity)
   check:verify(self._registered_handlers[name])

   local id = entity:get_id()
   for handler, _ in pairs(self._entity_msg_handlers[id]) do
      if self:_get_handler_name(handler) == name then
         self:destroy_msg_handler(handler)
      end
   end
end

function MsgDispatcher:destroy_msg_handler(handler)
   assert(env:is_running())
   assert(self:is_msg_handler(handler))

   self:call_handler(handler, 'radiant.md.destroy')
   -- xxx: for now leave the handler registered.  otherwise we can't
   -- stop behaviors after they've been destroyed.  the fix is to
   -- make behaviors actual objects (registered by name with the
   -- behavior manager) instead of msg handlers, and let it worry
   -- about lifetime.
   -- self._all_handlers[handler] = nil
end

function MsgDispatcher:listen(arg1, arg2, arg3)
   if util:is_string(arg1) then
      self:_listen_to_msg_filter(arg1, arg2)
      return
   end
   if util:is_a(arg1, Entity) then
      self:_listen_to_entity(arg1, arg2, arg3)
      return
   end
   check:report_error('invalid object type %s (%s) in md:listen', type(arg1), tostring(arg1))
end

function MsgDispatcher:_listen_to_msg_filter(filter, handler)
   check:verify(self:is_msg_filter(filter))
   --check:verify(self:is_msg_handler(handler));
   check:is_callable(handler)
   
   log:debug('listen %s (%s).', filter, self:_get_handler_name(handler))
   local handlers = self:_get_free_handlers_for_filter(filter)
   table.insert(handlers, handler)
end

function MsgDispatcher:_listen_to_entity(entity, filter, handler)
   check:is_entity(entity)
   check:verify(self:is_msg_filter(filter))
   check:is_callable(handler)
   
   if not entity:is_valid() then
      log:warning('ignoring invalid entity in md:listen')
      return
   end
   
   log:debug('listen entity %d, %s (%s).', entity:get_id(), filter, self:_get_handler_name(handler))

   local handlers = self:_get_entity_listen_map_by_filter(entity, filter)
   table.insert(handlers, handler)
end

function MsgDispatcher:unlisten(arg1, arg2, arg3)
   if util:is_string(arg1) then
      self:_unlisten_msg_filter(arg1, arg2)
      return
   end
   if util:is_a(arg1, Entity) then
      self:_unlisten_entity(arg1, arg2, arg3)
      return
   end
   check:report_error('invalid object type %s (%s) in md:listen', type(handler), tostring(handler))
end

function MsgDispatcher:_unlisten_msg_filter(filter, handler)
   check:verify(self:is_msg_filter(filter))

   local handlers = self:_get_free_handlers_for_filter(filter)
   for i, v in ipairs(handlers) do
      if v == handler then
         table.remove(handlers, i)
         return
      end
   end
end

function MsgDispatcher:_unlisten_entity(entity, filter, handler)
   check:is_entity(entity)
   check:verify(self:is_msg_filter(filter))
   --check:verify(self:is_msg_handler(handler))

   
   log:debug('unlisten_to_category entity %d, msgs %s (%s).',
              entity:get_id(), filter, self:_get_handler_name(handler))

   local handlers = self:_get_entity_listen_map_by_filter(entity, filter)
   for i, v in ipairs(handlers) do
      if v == handler then
         table.remove(handlers, i)
         return
      end
   end
   log:warning('could not find handler in unlisten!!')
end

function MsgDispatcher:_get_entity_listen_map_by_filter(entity, filter)
   check:is_entity(entity)
   check:verify(self:is_msg_filter(filter))

   local id = entity:get_id();
   local entity_handlers = self._entity_listen_map[id]
   if not entity_handlers then
      entity_handlers = {}
      self._entity_listen_map[id] = entity_handlers
   end
   local handlers = entity_handlers[filter]
   if not handlers then
      handlers = {}
      entity_handlers[filter] = handlers
   end
   -- log:debug('returning %d handlers for entity %d, category %s.', #handlers, id, category)
   return handlers
end

function MsgDispatcher:_get_free_handlers_for_filter(filter)
   local handlers = self._free_handlers[filter]
   if not handlers then
      handlers = {}
      self._free_handlers[filter] = handlers
   end
   -- log:debug('returning %d handlers for entity %d, category %s.', #handlers, id, category)
   return handlers
end

function MsgDispatcher:broadcast_msg(msg, ...)
   check:verify(self:is_msg(msg))

   local filters = self:_get_msg_filters(msg)

   for _, filter in ipairs(filters) do
      local handlers = self:_get_free_handlers_for_filter(filter)
      for _, handler in ipairs(handlers) do
         self:send_msg(handler, msg, ...)
      end
   end
end

-- use sparingly...
function MsgDispatcher:sync_send_msg(obj, msg, ...)
   self:_send_msg(true, obj, msg, ...)
end

function MsgDispatcher:send_msg(obj, msg, ...)
   self:_send_msg(false, obj, msg, ...)
end

function MsgDispatcher:_send_msg(sync, obj, msg, ...)
   check:is_boolean(sync)
   check:verify(util:is_callable(obj) or util:is_a(obj, Entity))
   check:verify(self:is_msg(msg))

   if not util:is_a(obj, Entity) then
      return self:_send_msg_to_handler(sync, obj, msg, ...)
   end
   return self:_send_msg_to_entity(sync, obj, msg, ...)
end

function MsgDispatcher:_send_msg_to_entity(sync, entity, msg, ...)  
   if not entity:is_valid() then
      log:warning('ignoring invalid entity in md:send_msg')
      return
   end

   local filters = self:_get_msg_filters(msg)

   for _, filter in ipairs(filters) do
      local handlers = self:_get_entity_listen_map_by_filter(entity, filter)
      for _i, handler in ipairs(handlers) do
         -- table constructors capture all return values from a function
         self:_send_msg_to_handler(sync, handler, msg, ...)
      end
   end
end

function MsgDispatcher:_get_handlers_for_filter(obj, filter)
   check:verify(self:is_msg_handler(handler) or util:is_a(obj, Entity))
   check:verify(self:is_msg_filter(filter))

   if util:is_a(obj, Entity) then
      return self:_get_entity_listen_map_by_filter(obj, filter)
   end
   if self:is_msg_handler(handler) then
      return self:_get_free_handlers_for_filter(obj, filter)      
   end
   check:report_error('invalid object type %s (%s) in md:_get_handlers_for_filter', type(obj), tostring(obj))
   return {}
end

function MsgDispatcher:_send_msg_to_handler(sync, handler, msg, ...)
   check:verify(self:is_msg(msg))

   if sync then
      self:call_handler(handler, msg, ...)
   else
      local entry = {
         handler = handler,
         msg = msg,
         args = {...},
      }
      table.insert(self._msg_queue, entry)
   end
end

function MsgDispatcher:_flush_msg_queue()
   local queue = self._msg_queue
   self._msg_queue = {}
   
   for i, entry in ipairs(queue) do
      self:call_handler(entry.handler, entry.msg, unpack(entry.args))
   end
end

function MsgDispatcher:call_handler(handler, msg, ...)
   -- if this assert fires, it's because an external agent is holding onto a 
   -- msg hander and calling call_handler on it directly.  this is illegal!
   -- they should be checking to make sure it's a handler first...
   assert(handler)
   
   check:verify(self:is_msg(msg))  
   log:debug('invoking (%s, %s, ...)', self:_get_handler_name(handler), msg)
   
   -- xxx - we should just be able to call handler(msg, ...) on clases with __call, right?
   if type(handler) == 'table' and type(handler.__index) == 'table' then
      local method = handler.__index[msg]
      if method then
         return method(handler, ...)
      end
      return nil
   end
   return handler(msg, ...)
end

function MsgDispatcher:_get_handler_name(handler)
   --check:verify(self:is_msg_handler(handler))
   local name = self._all_handlers[handler]
   return name and name or '-anonymous-'
end

print('-------------------------- LOADING MD ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------')
return MsgDispatcher()
