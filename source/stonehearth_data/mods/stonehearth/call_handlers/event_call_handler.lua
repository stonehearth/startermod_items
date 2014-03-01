
local datastore

local EventCallHandler = class()
function EventCallHandler:get_event_tracker(session, request)
   if not datastore then
      datastore = radiant.create_datastore()
      radiant.events.listen(stonehearth.events, 'stonehearth:new_event', function(e)
            self.__savestate:update({
                  text = e.text,
                  type = e.type,
               })
         end)
   end
   return { tracker = datastore }
end

return EventCallHandler