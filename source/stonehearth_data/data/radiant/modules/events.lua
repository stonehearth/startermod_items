local events = {}
local singleton = {}

function events.__init()
   singleton._registered_activities = {}    -- all possible activities, by name
   singleton._registered_handlers = {}      -- all registered msg handlers, by name
   singleton._registered_msgs = {}          -- all registered msgs, by name
   singleton._registered_msg_filters = {}   -- filters for all registered msgs, by name

   singleton._all_handlers = {}             -- translates handlers to their names
   singleton._entity_msg_handlers = {}      -- all msg handlers for all entities
   
   singleton._entity_listen_map = {}          -- listen map for handlers, by id
   singleton._free_handlers   = {}          -- all handlers who have called listen without an entity
   
   singleton._msg_queue = {}                -- list of messages yet to be sent.
   
   singleton._game_state_hooks = {}

   events.register_event('radiant.md.create')
   events.register_event('radiant.md.destroy')
   events.register_event('radiant.events.gameloop')
   events.register_event('radiant.commands.activate')
   
   events.register_event('radiant.events.slow_poll')   
   events.register_event('radiant.events.very_slow_poll')
   
   events.register_event('radiant.events.aura_expired')
end

function events._update()
   local now = radiant.gamestate.now()
   
   -- xxx: get rid of ALL polls.  register timers instead, or one time
   -- loop calls.
   
   events.broadcast_msg('radiant.events.gameloop', now)
   -- pump the polls
   if now % 200 == 0 then
      events.broadcast_msg('radiant.events.slow_poll', now)
   end  
   if now % 1000 == 0 then   
      events.broadcast_msg('radiant.events.very_slow_poll', now)
   end
   events._flush_msg_queue();
end

function events._call_game_hook(stage)
   local hooks = singleton._game_state_hooks[stage]
   if hooks then
      for cb, _ in pairs(hooks) do
         cb()
      end
   end
   singleton._game_state_hooks[stage] = {}
end

function events.notify_game_loop_stage(stage, handler)
   local hooks = singleton._game_state_hooks[stage]
   if not hooks then
      hooks = {}
      singleton._game_state_hooks[stage] = hooks
   end
   hooks[handler] = true
end

function events.is_msg(msg)
   return type(msg) == 'string' and singleton._registered_msgs[msg] ~= nil
end

function events.is_msg_filter(filter)
   return type(filter) == 'string' and singleton._registered_msg_filters[filter] ~= nil
end

function events.is_msg_handler(handler)
   return singleton._all_handlers[handler] ~= nil
end

-- xxx: make this a Lua Iterator (or better yet, cache em!)
function events._get_msg_filters(filter)
   local filters = {}
   table.insert(filters, filter)
   repeat
      filter = filter:gsub('(.*)%.(.*)', '%1') -- the lua regexp escape char is %.  nice...
      table.insert(filters, filter)
   until not filter:find('%.')
   return filters
end

function events.register_event(msg)
   radiant.log.info('registering msg "%s".', msg)
   radiant.check.verify(not events.is_msg_filter(msg))
   radiant.check.verify(not radiant.gamestate.is_running())

   singleton._registered_msgs[msg] = true
   for _, filter in ipairs(events._get_msg_filters(msg)) do
      singleton._registered_msg_filters[filter] = true
   end
end

function events.register_event_handler(name, handler)
   radiant.check.is_string(name)
   assert(not env:is_running()) 
   assert(not singleton._registered_handlers[name])
   radiant.check.is_callable(handler)
   
   radiant.log.info('registering msg_handler "%s".', name)
   singleton._registered_handlers[name] = handler
end

-- xxx: dangerous... how do these get loaded and saved?
function events.register_event_handler_instance(name, handler)
   radiant.check.is_string(name)
   assert(not singleton._registered_handlers[name])
   assert(not events.is_msg_handler(handler))
   singleton._all_handlers[handler] = name   
end

-- xxx: this belongs in the BehaviorManager, not the events
function events.register_activity(...)
   local name = select(1, ...)
   radiant.check.is_string(name)
   radiant.check.verify(not events.is_activity_name(name))

   radiant.log.debug('registering activity "%s".', name)
   singleton._registered_activities[name] = ...
end

-- xxx: this belongs in the BehaviorManager, not the events
function events.is_activity(a)
   return type(a) == 'table' and events.is_activity_name(a.name)
end

-- xxx: this belongs in the BehaviorManager, not the events
function events.is_activity_name(name)
   return type(name) == 'string' and singleton._registered_activities[name]
end


-- xxx: this belongs in the BehaviorManager, not the events
function events.create_activity(...)
   local activity_name = select(1, ...)
   radiant.check.verify(events.is_activity_name(activity_name))

   radiant.log.debug('creating activity "%s".', activity_name)
   local result = {
      name = activity_name,
      args = {select(2, ...)}
   }
   return result;
end

function events.create_msg_handler(name, ...)
   assert(env:is_running()) 
   assert(singleton._registered_handlers[name])
   assert(name)
   
   local ctor = singleton._registered_handlers[name]
   assert(ctor)

   local handler = ctor();
   handler = handler and handler or ctor
   singleton._all_handlers[handler] = name   
   events.call_handler(handler, 'radiant.md.create', ...)

   return handler
end

function events.add_msg_handler(entity, name, ...)
   radiant.check.is_entity(entity)
   radiant.check.verify(singleton._registered_handlers[name])
   
   local handler = events.create_msg_handler(name, entity, ...) 

   -- track which entity the handler is attached to so we can implement
   -- send_msg, save, load, etc.
   local id = entity:get_id();
   if not singleton._entity_msg_handlers[id] then
      singleton._entity_msg_handlers[id] = {}
   end  
   singleton._entity_msg_handlers[id][handler] = true
     
   return handler
end

function events.remove_msg_handler(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.verify(singleton._registered_handlers[name])

   local id = entity:get_id()
   for handler, _ in pairs(singleton._entity_msg_handlers[id]) do
      if events._get_handler_name(handler) == name then
         events.destroy_msg_handler(handler)
      end
   end
end

function events.destroy_msg_handler(handler)
   assert(env:is_running())
   assert(events.is_msg_handler(handler))

   events.call_handler(handler, 'radiant.md.destroy')
   -- xxx: for now leave the handler registered.  otherwise we can't
   -- stop behaviors after they've been destroyed.  the fix is to
   -- make behaviors actual objects (registered by name with the
   -- behavior manager) instead of msg handlers, and let it worry
   -- about lifetime.
   -- singleton._all_handlers[handler] = nil
end

function events.listen(arg1, arg2, arg3)
   if radiant.util.is_string(arg1) then
      events._listen_to_msg_filter(arg1, arg2)
      return
   end
   if radiant.util.is_a(arg1, Entity) then
      events._listen_to_entity(arg1, arg2, arg3)
      return
   end
   radiant.check.report_error('invalid object type %s (%s) in md:listen', type(arg1), tostring(arg1))
end

function events._listen_to_msg_filter(filter, handler)
   radiant.check.verify(events.is_msg_filter(filter))
   --radiant.check.verify(events.is_msg_handler(handler));
   radiant.check.is_callable(handler)
   
   radiant.log.debug('listen %s (%s).', filter, events._get_handler_name(handler))
   local handlers = events._get_free_handlers_for_filter(filter)
   table.insert(handlers, handler)
end

function events._listen_to_entity(entity, filter, handler)
   radiant.check.is_entity(entity)
   radiant.check.verify(events.is_msg_filter(filter))
   radiant.check.is_callable(handler)
   
   if not entity:is_valid() then
      log:warning('ignoring invalid entity in md:listen')
      return
   end
   
   radiant.log.debug('listen entity %d, %s (%s).', entity:get_id(), filter, events._get_handler_name(handler))

   local handlers = events._get_entity_listen_map_by_filter(entity, filter)
   table.insert(handlers, handler)
end

function events.unlisten(arg1, arg2, arg3)
   if radiant.util.is_string(arg1) then
      events._unlisten_msg_filter(arg1, arg2)
      return
   end
   if radiant.util.is_a(arg1, Entity) then
      events._unlisten_entity(arg1, arg2, arg3)
      return
   end
   radiant.check.report_error('invalid object type %s (%s) in md:listen', type(handler), tostring(handler))
end

function events._unlisten_msg_filter(filter, handler)
   radiant.check.verify(events.is_msg_filter(filter))

   local handlers = events._get_free_handlers_for_filter(filter)
   for i, v in ipairs(handlers) do
      if v == handler then
         table.remove(handlers, i)
         return
      end
   end
end

function events._unlisten_entity(entity, filter, handler)
   radiant.check.is_entity(entity)
   radiant.check.verify(events.is_msg_filter(filter))
   --radiant.check.verify(events.is_msg_handler(handler))

   
   radiant.log.debug('unlisten_to_category entity %d, msgs %s (%s).',
              entity:get_id(), filter, events._get_handler_name(handler))

   local handlers = events._get_entity_listen_map_by_filter(entity, filter)
   for i, v in ipairs(handlers) do
      if v == handler then
         table.remove(handlers, i)
         return
      end
   end
   log:warning('could not find handler in unlisten!!')
end

function events._get_entity_listen_map_by_filter(entity, filter)
   radiant.check.is_entity(entity)
   radiant.check.verify(events.is_msg_filter(filter))

   local id = entity:get_id();
   local entity_handlers = singleton._entity_listen_map[id]
   if not entity_handlers then
      entity_handlers = {}
      singleton._entity_listen_map[id] = entity_handlers
   end
   local handlers = entity_handlers[filter]
   if not handlers then
      handlers = {}
      entity_handlers[filter] = handlers
   end
   -- radiant.log.debug('returning %d handlers for entity %d, category %s.', #handlers, id, category)
   return handlers
end

function events._get_free_handlers_for_filter(filter)
   local handlers = singleton._free_handlers[filter]
   if not handlers then
      handlers = {}
      singleton._free_handlers[filter] = handlers
   end
   -- radiant.log.debug('returning %d handlers for entity %d, category %s.', #handlers, id, category)
   return handlers
end

function events.broadcast_msg(msg, ...)
   radiant.check.verify(events.is_msg(msg))

   local filters = events._get_msg_filters(msg)

   for _, filter in ipairs(filters) do
      local handlers = events._get_free_handlers_for_filter(filter)
      for _, handler in ipairs(handlers) do
         events.send_msg(handler, msg, ...)
      end
   end
end

-- use sparingly...
function events.sync_send_msg(obj, msg, ...)
   events._send_msg(true, obj, msg, ...)
end

function events.send_msg(obj, msg, ...)
   events._send_msg(false, obj, msg, ...)
end

function events._send_msg(sync, obj, msg, ...)
   radiant.check.is_boolean(sync)
   radiant.check.verify(radiant.util.is_callable(obj) or radiant.util.is_a(obj, Entity))
   radiant.check.verify(events.is_msg(msg))

   if not radiant.util.is_a(obj, Entity) then
      return events._send_msg_to_handler(sync, obj, msg, ...)
   end
   return events._send_msg_to_entity(sync, obj, msg, ...)
end

function events._send_msg_to_entity(sync, entity, msg, ...)  
   if not entity:is_valid() then
      log:warning('ignoring invalid entity in md:send_msg')
      return
   end

   local filters = events._get_msg_filters(msg)

   for _, filter in ipairs(filters) do
      local handlers = events._get_entity_listen_map_by_filter(entity, filter)
      for _i, handler in ipairs(handlers) do
         -- table constructors capture all return values from a function
         events._send_msg_to_handler(sync, handler, msg, ...)
      end
   end
end

function events._get_handlers_for_filter(obj, filter)
   radiant.check.verify(events.is_msg_handler(handler) or radiant.util.is_a(obj, Entity))
   radiant.check.verify(events.is_msg_filter(filter))

   if radiant.util.is_a(obj, Entity) then
      return events._get_entity_listen_map_by_filter(obj, filter)
   end
   if events.is_msg_handler(handler) then
      return events._get_free_handlers_for_filter(obj, filter)      
   end
   radiant.check.report_error('invalid object type %s (%s) in md:_get_handlers_for_filter', type(obj), tostring(obj))
   return {}
end

function events._send_msg_to_handler(sync, handler, msg, ...)
   radiant.check.verify(events.is_msg(msg))

   if sync then
      events.call_handler(handler, msg, ...)
   else
      local entry = {
         handler = handler,
         msg = msg,
         args = {...},
      }
      table.insert(singleton._msg_queue, entry)
   end
end

function events._flush_msg_queue()
   local queue = singleton._msg_queue
   singleton._msg_queue = {}
   
   for i, entry in ipairs(queue) do
      events.call_handler(entry.handler, entry.msg, unpack(entry.args))
   end
end

function events.call_handler(handler, msg, ...)
   -- if this assert fires, it's because an external agent is holding onto a 
   -- msg hander and calling call_handler on it directly.  this is illegal!
   -- they should be checking to make sure it's a handler first...
   assert(handler)
   
   radiant.check.verify(events.is_msg(msg))  
   radiant.log.debug('invoking (%s, %s, ...)', events._get_handler_name(handler), msg)
   
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

function events._get_handler_name(handler)
   --check:verify(events.is_msg_handler(handler))
   local name = singleton._all_handlers[handler]
   return name and name or '-anonymous-'
end

events.__init()
return events
