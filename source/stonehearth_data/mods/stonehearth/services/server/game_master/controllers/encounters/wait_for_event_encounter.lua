
local WaitForEventEncounter = class()

function WaitForEventEncounter:initialize()
   self._log = radiant.log.create_logger('game_master.encounters.wait_for_event')
end

function WaitForEventEncounter:start(ctx, info)
   local event = info.event
   local source = info.source

   assert(source)
   assert(event)

   source = ctx[source]
   self._sv.source = source
   self._sv.event = event
   self.__saved_variables:mark_changed()

   self._log:debug('listening for "%s" event on "%s"', event, tostring(source))
   self._listener = radiant.events.listen(source, event, function()
         self._log:debug('"%s" event on "%s" triggered!', tostring(source), event)
         ctx.arc:trigger_next_encounter(ctx)         
      end)
end

function WaitForEventEncounter:stop()
   if self._listener then
      self._listener:destroy()
      self._listener = nil
   end
end

return WaitForEventEncounter

