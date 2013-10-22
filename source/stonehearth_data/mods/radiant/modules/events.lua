local events = {}

function events.__init()
   events._senders = {}
end

function events.listen(object, event, self, fn)
   assert(object and event and self and fn)

   if not events._senders[object] then
      -- add the sender
      events._senders[object] = {}
   end

   local sender = events._senders[object]

   if not sender[event] then
      sender[event] = {}
   end

   local listeners = sender[event]

   radiant.log.info('listening to event ' .. event)
   
   table.insert(listeners, { self = self, fn = fn })
end

function events.unlisten(object, event, self, fn)
   assert(object and event and self and fn)   
   assert(events._senders[object])
   assert(events._senders[object][event])
   
   local listeners = events._senders[object][event]

   for i, listener in ipairs(listeners) do
      if listener.fn == fn then
         radiant.log.info('unlistening to event ' .. event)
         table.remove(listeners, i)
         break
      end
   end
end

function events.trigger(object, event, ...)
   local sender = events._senders[object]
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
   
   events.trigger(radiant.events, 'gameloop', now)
   -- pump the polls
   if now.now % 200 == 0 then
      events.trigger(radiant.events, 'slow_poll', now)
   end
   if now.now % 1000 == 0 then
      events.trigger(radiant.events, 'very_slow_poll', now)
   end
end


function events.register_event_handler()
   radiant.log.warning('this is defunct. Delete the caller')
end

events.__init()
return events
