local Point3 = _radiant.csg.Point3

local AnalyticsCallHandler = class()

--- Send a design event to the server.
--  @param event_name
--  @param value - float related to the event name
--  @param position - coordinates of target position
--  @param area - string describing more detail about the event
function AnalyticsCallHandler:send_design_event(session, response, event_name, value, position, area)
   local event = _radiant.analytics.DesignEvent(event_name)
   
   if value then
      event:set_value(data.value)
   end

   if position then
      event:set_position(Point3(data.position.x, data.position.y, data.position.z))
   end

   if area then
      event:set_area(data.area)
   end

   event:send_event()

   return true
end

return AnalyticsCallHandler