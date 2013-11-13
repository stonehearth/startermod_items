local events = {}
local singleton = {
   jobs = {}
}

function events.__init()
   events._senders = {}
end

function events._convert_object_to_key(object)
   if type(object) == 'userdata' then
      assert(object.get_id, 'could not convert userdata object to key.  no get_id method implemented')
      return 'userdata_id_' .. tostring(object:get_id())
   end
   return object
end

function events.listen(object, event, self, fn)
   assert(object and event and self and fn)

   local key = events._convert_object_to_key(object)
   if not events._senders[key] then
      -- add the sender
      events._senders[key] = {}
   end

   local sender = events._senders[key]
   if not sender[event] then
      sender[event] = {}
   end

   local listeners = sender[event]

   radiant.log.info('listening to event ' .. event)
   
   table.insert(listeners, { self = self, fn = fn })
end

function events.unlisten(object, event, self, fn)
   local key = events._convert_object_to_key(object)

   assert(object and event and self and fn)
   local senders = events._senders[key]
   if not senders then
      radiant.log.warning('unlisten %s on unknown sender: %s', event, tostring(object))
      return
   end
   local listeners = senders[event]
   if not listeners then
      radiant.log.warning('unlisten unknown event: %s on sender %s', event, tostring(object))
      return
   end

   for i, listener in ipairs(listeners) do
      if listener.fn == fn and listener.self == self then
         radiant.log.info('unlistening to event ' .. event)
         table.remove(listeners, i)
         return
      end
   end
   radiant.log.warning('unlisten could not find registered listener for event: %s', event)
end

function events.trigger(object, event, ...)
   local key = events._convert_object_to_key(object)
   local sender = events._senders[key]

   if sender then
      local listeners = sender[event]
      if listeners then
         for i, listener in ipairs(listeners) do
            -- build up the event object
            local event_params = {}
            event_params.sender = sender
            event_params.event = event
            
            if ... then 
               for name, value in pairs(...) do
                  if name ~= 'sender' and name ~= 'event' then
                     event_params[name] = value
                  end
               end
            end

            radiant.log.info('triggering event ' .. event)
            listener.fn(listener.self, event_params)
         end
      end
   end
end

function events._update()
   local now = { now = radiant.gamestate.now() }
   
   events.trigger(radiant.events, 'stonehearth:gameloop', now)
   -- pump the polls
   if now.now % 200 == 0 then
      events.trigger(radiant.events, 'stonehearth:slow_poll', now)
   end
   --Fires once a second
   if now.now % 1000 == 0 then
      events.trigger(radiant.events, 'stonehearth:very_slow_poll', now)
   end
   --Fires once a minute
   if now.now % 60000 == 0 then
      events.trigger(radiant.events, 'stonehearth:minute_poll', now)
   end
   --Fires every 10 minutes
   if now.now % 600000 == 0 then
      events.trigger(radiant.events, 'stonehearth:ten_minute_poll', now)
   end
end


function events.register_event_handler()
   radiant.log.warning('this is defunct. Delete the caller')
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
