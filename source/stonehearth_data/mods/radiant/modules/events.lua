local events = {
   UNLISTEN = { 'remove installed hook' }
}
local singleton = {
   jobs = {}
}

local trigger_depth = 0
local log = radiant.log.create_logger('events')

function events.__init()
   events._senders = {}
   events._async_triggers = {}
   events._dead_listeners = {}
end

function events.create_listeners()   
   -- These events should be deleted and each client should just set a calendar or real-time timer
   -- according to their needs. Using the events below just bunches up the processing onto a single
   -- gameloop causing periodic stuttering.

   -- Fires five times a second.   
   radiant.set_realtime_interval("(not saved) Radiant Listener slow_poll", 200, function()
         local now = { now = radiant.gamestate.now() }
         events.trigger(radiant, 'stonehearth:slow_poll', now)
      end)

   --Fires once a second.
   radiant.set_realtime_interval("(not saved) Radiant Listener very_slow_poll", 1000, function()
         local now = { now = radiant.gamestate.now() }
         events.trigger(radiant, 'stonehearth:very_slow_poll', now)
      end)

   --Fires once a minute.
   radiant.set_realtime_interval("(not saved) Radiant Listener minute_poll", 1000 * 60, function()
         local now = { now = radiant.gamestate.now() }
         events.trigger(radiant, 'stonehearth:minute_poll', now)
      end)
end   

function events._convert_object_to_key(object)
   if type(object) == 'userdata' then
      assert(object.get_id, 'could not convert userdata object to key.  no get_id method implemented')
      if object.is_valid then
         if not object:is_valid() then
            return nil
         end
      end
      return 'userdata_id_' .. tostring(object:get_id())
   end
   return object
end

function events.listen_once(object, event, obj, method)
   return events.listen(object, event, function(...)
         if method then
            method(obj, ...)
         else
            obj(...)
         end
         return radiant.events.UNLISTEN
      end)
end

-- takes 2 forms:
-- radiant.events.listen(sender, 'event_name', object, method)
-- radiant.events.listen(sender, 'event_name', function)
function events.listen(object, event, self, fn)
   assert(object)
   assert(event)
   assert(self)

   local key = events._convert_object_to_key(object)
   if not events._senders[key] then
      -- add the sender
      events._senders[key] = {}
   end

   local sender = events._senders[key]
   local listeners = sender[event]
   if not listeners then
      -- `listeners` is a numeric array of all the people listenting to the event.
      -- still, keep a `trigger_depth` count of how deep we are in a stack of
      -- lua functions triggering this event so we can avoid modifying the
      -- listener list in the middle of a trigger
      listeners = {
         trigger_depth = 0
      }
      sender[event] = listeners
   end

   log:spam('listening to event ' .. event)  

   local resurrected = false
   for i, dead_entry in ipairs(events._dead_listeners) do
      if dead_entry.event == event and dead_entry.key == key and dead_entry.fn == fn and dead_entry.self == self then
         table.remove(events._dead_listeners, i)
         dead_entry.entry.dead = false
         resurrected = true
         break
      end
   end

   if not resurrected then
      local entry = {
         dead = false,
         self = self,
         fn = fn
      }

      table.insert(listeners, entry)
   else
      log:spam('resurrecting listener for ' .. event)
   end

   return radiant.lib.Destructor(function()
         events._unlisten(object, key, event, self, fn)
      end)
end

function events.unpublish(object)
   local key = events._convert_object_to_key(object)

   assert(object)
   if not events._senders[key] then
      log:debug('unpublish on unknown sender: %s', tostring(object))
      return
   end
   log:debug('forcibly removing listeners while unpublishing %s', key)
   events._senders[key] = nil
end

function events._clear_listener(i, key, event)
   local senders = events._senders[key]
   local listeners = senders[event]

   log:spam('unlistening to event ' .. event)

   table.remove(listeners, i)

   if #listeners == 0 then
      senders[event] = nil
      if next(events._senders[key]) == nil then
         events._senders[key] = nil
      end
   end
end

function events._mark_listener_dead(entry, object, key, event)
   log:spam('queuing dead event ' .. event)
   entry.dead = true
   local dead_entry = {
      entry = entry,
      object = object,
      key = key,
      event = event,
      self = entry.self,
      fn = entry.fn
   }
   table.insert(events._dead_listeners, dead_entry)
end

function events._unlisten(object, key, event, self, fn)
   local senders = events._senders[key]
   if not senders then
      log:info('unlisten %s on unknown sender: %s', event, tostring(object))
      return
   end

   local listeners = senders[event]
   if not listeners then
      log:info('unlisten unknown event: %s on sender %s', event, tostring(object))
      return
   end

   for i, entry in ipairs(listeners) do
      if entry.fn == fn and entry.self == self and not entry.dead then
         if trigger_depth > 0 then
            events._mark_listener_dead(entry, object, key, event)
         else
            events._clear_listener(i, key, event)
         end
         return
      end
   end
   log:warning('unlisten could not find registered listener for event: %s', event)
end

function events.trigger_async(object, event, ...)
   local trigger = {
      object = object,
      event = event,
      args = { ... },
   }
   table.insert(events._async_triggers, trigger)
end

-- report the current stack to the host so we can log it
function events._trigger_error_handler(err)
   local traceback = debug.traceback()
   _host:report_error(err, traceback)
   return err   
end

function events.trigger(object, event, ...)
   if log:is_enabled(radiant.log.SPAM) then
      log:spam('triggering %s', tostring(event))
   end
   
   local key = events._convert_object_to_key(object)
   local sender = events._senders[key]
   local args = { ... }

   if sender then
      local listeners = sender[event]
      if listeners then
         log:debug('trigging %d listeners for "%s"', #listeners, event)

         -- do the actual triggering a pcall to make sure all the triggers get
         -- hit and our trigger_depth counter doesn't get screwed up
         trigger_depth = trigger_depth + 1
         for _, entry in ipairs(listeners) do
            local result = nil
            if not entry.dead then
               -- do the actual call in an xpcall to make sure our trigger depth stays
               -- in sync.
               xpcall(function()
                     if entry.fn ~= nil then
                     -- type 1: listen was called with 'self' and a method to call
                        result = entry.fn(entry.self, unpack(args))
                     else
                        -- type 2: listen was called with just a function!  call it
                        result = entry.self(unpack(args))
                     end
                  end, events._trigger_error_handler)
            end

            if result == radiant.events.UNLISTEN then
               events._mark_listener_dead(entry, object, key, event)
            end
         end

         trigger_depth = trigger_depth - 1
      end
   end
end

function events._update()
   assert(trigger_depth == 0)
   
   local async_triggers = events._async_triggers
   events._async_triggers = {}
   for _, trigger in ipairs(async_triggers) do
      events.trigger(trigger.object, trigger.event, unpack(trigger.args))
   end

   -- Fires every update--by default, every 50 ms.
   local now = { now = radiant.gamestate.now() }
   events.trigger(radiant, 'stonehearth:gameloop', now)

   assert(trigger_depth == 0)
   local oldsize = #events._dead_listeners
   for _, entry in ipairs(events._dead_listeners) do
      log:spam('removing dead-list event %s', entry.event)
      entry.entry.dead = false
      events._unlisten(entry.object, entry.key, entry.event, entry.self, entry.fn)
   end
   assert(#events._dead_listeners == oldsize)
   events._dead_listeners = {}
end

function radiant.create_background_task(name, fn)
   local co = coroutine.create(fn)
   local thread_main = function()
      local success, _ = coroutine.resume(co)
      if not success then
         radiant.check.report_thread_error(co, 'co-routine failed: ' .. tostring(_))
         return false
      end
      local status = coroutine.status(co)
      if status == 'suspended' then
         return true
      end
      return false
   end
   local job = _radiant.sim.create_job(name, thread_main)
   table.insert(singleton.jobs, job)
end

events.__init()

return events
