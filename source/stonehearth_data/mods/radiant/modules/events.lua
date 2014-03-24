local events = {
   UNLISTEN = { 'remove installed hook' }
}
local singleton = {
   jobs = {}
}

local log = radiant.log.create_logger('events')

function events.__init()
   events._senders = {}
   events._async_triggers = {}
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

-- takes 2 forms:
-- radiant.events.listen(sender, 'event_name', object, method)
-- radiant.events.listen(sender, 'event_name', function)

function events.listen(object, event, self, fn)
   assert(object and event and self)

   local key = events._convert_object_to_key(object)
   if not events._senders[key] then
      -- add the sender
      events._senders[key] = {}
   end

   local sender = events._senders[key]
   local listeners = sender[event]
   if not listeners then
      listeners = {}
      sender[event] = listeners
   end


   log:spam('listening to event ' .. event)  
   local entry = {}   
   if fn then
      -- this is a method on an object.  theoretical question:  what
      -- happens if an object instance listens for an event and forgets
      -- to unlisten before the last reference to it goes away?  we
      -- don't want to be the only one keeping that object alive, and we certainly
      -- don't want to fire notifications into the ether that are not 
      -- useful (which presumedly these are, since the object should have
      -- been garbage collected!)   To prevent this, use a weak table to
      -- make sure the object gets collected if this is the very very last
      -- reference to it.
      setmetatable(entry, { __mode = 'kv' })
      entry.self = self
      entry.fn = fn
   else
      -- self is actually just a function
      entry.self = self
   end
   table.insert(listeners, entry)
end

function events.unpublish(object)
   local key = events._convert_object_to_key(object)

   assert(object)
   if not events._senders[key] then
      log:debug('unpublish on unknown sender: %s', tostring(object))
      return
   end
   log:debug('forcibly removing listeners while unpublishing %s')
   events._senders[key] = nil
end

function events.unlisten(object, event, self, fn)
   local key = events._convert_object_to_key(object)

   assert(object and event and self)
   local senders = events._senders[key]
   if not senders then
      log:debug('unlisten %s on unknown sender: %s', event, tostring(object))
      return
   end
   local listeners = senders[event]
   if not listeners then
      log:debug('unlisten unknown event: %s on sender %s', event, tostring(object))
      return
   end

   for i, listener in ipairs(listeners) do
      if listener.fn == fn and listener.self == self then
         log:spam('unlistening to event ' .. event)
         table.remove(listeners, i)
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

function events.trigger(object, event, ...)
   log:spam('triggering %s', tostring(event))
   
   local key = events._convert_object_to_key(object)
   local sender = events._senders[key]

   if sender then
      local listeners = sender[event]
      if listeners then
         log:debug('trigging %d listeners for "%s"', #listeners, event)
         local i = 1
         while i <= #listeners do
            local entry = listeners[i]
            local result
            if not entry.self then
               -- the object got garbage collected before it could unlisten.  it's
               -- probably buggy, but let's not let that bother us too much.  just
               -- remove it from the array.
               log:detail('   listener %d has been collected.  not firing.', i)
               result = radiant.events.UNLISTEN
            else
               if entry.fn ~= nil then
               -- type 1: listen was called with 'self' and a method to call
                  result = entry.fn(entry.self, ...)
               else
                  -- type 2: listen was called with just a function!  call it
                  result = entry.self(...)
               end
            end

            if result == radiant.events.UNLISTEN then
               log:detail('   removing listener index %d (count:%d)', i, #listeners)
               table.remove(listeners, i)
            else
               log:detail('   advancing index %d (count:%d)', i, #listeners)
               i = i + 1
            end
         end
      end
   end
end

function events._update()
   local now = { now = radiant.gamestate.now() }
   
   local async_triggers = events._async_triggers
   events._async_triggers = {}
   for _, trigger in ipairs(async_triggers) do
      events.trigger(trigger.object, trigger.event, unpack(trigger.args))
   end

   events.trigger(radiant, 'stonehearth:gameloop', now)
   -- pump the polls
   if now.now % 200 == 0 then
      events.trigger(radiant, 'stonehearth:slow_poll', now)
   end
   --Fires once a second
   if now.now % 1000 == 0 then
      events.trigger(radiant, 'stonehearth:very_slow_poll', now)
   end
   --Fires once a minute
   if now.now % 60000 == 0 then
      events.trigger(radiant, 'stonehearth:minute_poll', now)
   end
   --Fires every 10 minutes
   if now.now % 600000 == 0 then
      events.trigger(radiant, 'stonehearth:ten_minute_poll', now)
   end
end

radiant.create_background_task = function(name, fn)
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
